#pragma once

#include <map>

#include <storage/Utils.hpp>
#include <storage/IntervalMap.hpp>
#include <storage/StorageHelpers.hpp>
#include <storage/SerializeImpl.hpp>
#include <storage/DataStorage.hpp>
#include <storage/RandomMemoryAccess.hpp>

/*
    get address -> alloc_unmapped() -> add to unmapped
    stop using -> del() -> remove from unmapped or intervals
    start using -> lookup() -> alloc() -> add to intervals, find new range in empty or at the end
    using -> lookup() -> find in m
    expanding address -> expand() -> allocate new mapping alloc() or expand existing m entry
*/
class VirtAddressMapping{
protected:
    IntervalMap<uint64_t, size_t> intervals; //addr -> <offset, size>
    std::map<uint64_t, size_t> unmapped;
    SetOrderedBySize<size_t, size_t> empty;
    std::pair<uint64_t, size_t> max; //virtaddr, offset

    static constexpr size_t MIN_EMPTYSIZE = 16;
    static constexpr size_t UNMAPPED_OFFSET = std::numeric_limits<size_t>::max();

    size_t alloc_offset(size_t addr, size_t size){
        auto advance_offset = [](size_t &offset, size_t size) -> size_t{
                auto old_offset = offset;
                offset = offset + size;
                return old_offset;
            };
        auto [has, offset] = empty.splice(size, advance_offset);
        if(has){
            return offset;
        }
        // allocate new range at the end
        max.first = addr + size;
        max.second += size;
        LOG_DEBUG("VirtAddressMapping::alloc_offset {%lx, %lu}->[%lu, %lu]",
            addr, size, max.second - size, size);
        return max.second - size;
    }
    static uint64_t end_offset(uint64_t addr, uint64_t size){
        ASSERT_ON(size == 0);
        return addr + size - 1;
    }
    static uint64_t size_from_start_end(uint64_t start, uint64_t end){
        return end - start + 1;
    }
public:
    static constexpr size_t DEFAULT_SIZE = 1024 * 1024;
    static constexpr size_t EXTRA_SPARE_SPACE = 1024;

    //lookup for mapping addres -> <offset, size>
    std::pair<size_t, size_t> lookup(uint64_t addr, size_t size, bool is_read_only = false) {
        auto iv_result = intervals.find(addr);
        ASSERT_ON(!iv_result.found());
        auto [va_start, va_end] = iv_result.range();
        auto va_size = size_from_start_end(va_start, va_end);
        auto addr_offset = addr - va_start;
        auto mapped_offset = iv_result.value();
        ASSERT_ON(va_start > addr);
        ASSERT_ON(va_end < end_offset(addr, size));

        if(mapped_offset == UNMAPPED_OFFSET){
            ASSERT_ON(is_read_only);
            iv_result.value() = mapped_offset = alloc_offset(addr, size);
        }

        LOG_DEBUG("VirtAddressMapping::lookup {%lx, %lu}->[%lu, %lu]",
            addr, size, mapped_offset + addr_offset, va_size - addr_offset);
        LOG_DEBUG("VirtAddressMapping::lookup va_size=%lu addr_offset=%ld va_start=%lx addr=%lx",
            va_size, addr_offset, va_start, addr);
        return std::make_pair(mapped_offset + addr_offset,
            va_size - addr_offset);
    }
    //alloc new range
    size_t alloc(uint64_t addr, size_t size){
        size_t offset = alloc_offset(addr, size);
        auto res = intervals.insert(addr, end_offset(addr, size), offset);
        ASSERT_ON(res != Result::Success);
        return offset;
    }

    //when we get new addres, or delete range
    Result alloc_unmapped(uint64_t addr, size_t size){
        auto res = intervals.insert(addr, end_offset(addr, size), UNMAPPED_OFFSET);
        return res;
    }

    //unmap [addr, addr + size]
    Result del(uint64_t addr, size_t size){
        size_t count = 0;
        auto add_to_empty = [&]([[maybe_unused]] auto iv_result, uint64_t va_start, uint64_t va_end, size_t offset){
            auto va_size = size_from_start_end(va_start, va_end);
            if (offset == UNMAPPED_OFFSET || va_size == 0){
                return;
            }
            count++;
            empty.add(va_size, offset); //TODO merge neighbours. maybe via ranges?
        };
        intervals.for_each_interval(addr, end_offset(addr, size), std::move(add_to_empty));
        RET_ON_FAIL(!count);
        auto res = intervals.set(addr, end_offset(addr, size), UNMAPPED_OFFSET);

        return res;
    }
    Result expand(StorageAddress address, size_t size){
        //allocate new range with StorageAddress{address.addr + addr.size, size};
        if(max.first == address.addr + address.size){
            //expand end of file
            auto expand_offset = [this](decltype(intervals)::IntervalResult &iv_result, size_t old_end, size_t new_end){
                if(!iv_result.found()){
                    return;
                }
                auto offset = iv_result.value();
                auto size = new_end - old_end;
                offset += size;
                max.first += size;
                max.second += size;
            };
            auto res = intervals.expand(address.addr, end_offset(address.addr, address.size), expand_offset);
            RET_ON_SUCCESS(res);
            //failed to expand, just allocate new
        }
        //otherwise allocane new mapping
        alloc(address.addr + address.size, size);
        return Result::Success;
    }

    //only sequential support
    Result serializeImpl(StorageBuffer<> &buffer) const{
        ASSERT_ON(getSizeImpl() > buffer.allocated());
        size_t offset = 0;
        auto buf = buffer.offset_advance(offset, szeimpl::size(intervals));
        szeimpl::s(intervals, buf);

        buf = buffer.offset_advance(offset, szeimpl::size(unmapped));
        szeimpl::s(unmapped, buf);

        buf = buffer.offset_advance(offset, szeimpl::size(empty));
        szeimpl::s(empty, buf);

        buf = buffer.offset_advance(offset, szeimpl::size(max));
        szeimpl::s(max, buf);

        return Result::Success;
    }

    static VirtAddressMapping deserializeImpl(const StorageBufferRO<> &buffer) {
        VirtAddressMapping vam;

        size_t offset = 0;
        auto buf = buffer;

        vam.intervals = szeimpl::d<decltype(vam.intervals)>(buf);
        buf = buffer.advance_offset(offset, szeimpl::size(vam.intervals));

        vam.unmapped = szeimpl::d<decltype(vam.unmapped)>(buf);
        buf = buffer.advance_offset(offset, szeimpl::size(vam.unmapped));

        vam.empty = szeimpl::d<decltype(vam.empty)>(buf);
        buf = buffer.advance_offset(offset, szeimpl::size(vam.empty));

        vam.max = szeimpl::d<decltype(vam.max)>(buf);

        return vam;
    }

    size_t getSizeImpl() const {
        size_t size = 0;
        size += szeimpl::size(intervals);
        size += szeimpl::size(unmapped);
        size += szeimpl::size(empty);
        size += szeimpl::size(max);
        return size;
    }
};

template<CRMA R>
class SimpleStorage: public DataStorageBase{
    R rma;

    //address space => file mapping
    //mapping -> at the end of the file when close
    //Short metadate in the beginning of the file of fixed size
    static constexpr uint32_t MAGIC = 0xFE2B0CC7;
    struct FileMetadata{
        uint32_t magic;
        RandomAddressRange<> ra;
        size_t va_offset, va_size;
        uint64_t va_addr;
        StorageAddress static_addr;
    };
    FileMetadata metadata;

    VirtAddressMapping mapping;

    void init_simple_storage(size_t static_size){
        StorageBufferRO metadata_buf{rma.readb(0, sizeof(FileMetadata))};
        const FileMetadata *fm = metadata_buf.template get<FileMetadata>();
        if(fm->magic != MAGIC){
            //new file!
            metadata.magic = MAGIC;
            metadata.va_addr = metadata.ra.get_random_address(VirtAddressMapping::DEFAULT_SIZE);
            uint64_t metadata_addr = metadata.ra.get_random_address(szeimpl::size(metadata));
            //first mapping for the beginning of the file
            auto offset = mapping.alloc(metadata_addr, szeimpl::size(metadata));
            ASSERT_ON(offset != 0);
            metadata.static_addr = get_random_address(static_size);
            initialize_zero(*this, metadata.static_addr);
        } else {
            metadata = *fm;
            ASSERT_ON(metadata.static_addr.size < static_size);
            StorageBufferRO vmap_buf{rma.readb(metadata.va_offset, metadata.va_size)};
            mapping = szeimpl::d<decltype(mapping)>(vmap_buf);
            mapping.del(metadata.va_addr, metadata.va_size);
        }
    }
public:
    SimpleStorage(R &&rma, size_t static_size, uint32_t id): DataStorageBase(id), rma(std::move(rma)) {
        init_simple_storage(static_size);
    }
    SimpleStorage(R &&rma, uint32_t id = DataStorageBase::UniqueIDInterface::DEFAULT): DataStorageBase(id), rma(std::move(rma)) {
        init_simple_storage(sizeof(StaticHeader));
    }

    StorageAddress get_static_section()  {
        return metadata.static_addr;
    }

    // find random address from definately unused blocks
    StorageAddress get_random_address(size_t size)  {
        uint64_t addr = metadata.ra.get_random_address(size);
        mapping.alloc_unmapped(addr, size);
        return StorageAddress{addr, size};
    }

    // expand existing address range
    StorageAddress expand_address(const StorageAddress &address, size_t size)  {
        mapping.expand(address, size);
        return StorageAddress{address.addr + address.size, size};
    }

    Result write(const StorageAddress &addr, const StorageBuffer<> &buffer)  {
        auto [offset, size] = mapping.lookup(addr.addr, addr.size);
        LOG_INFO("Storage::write [%lu,%lu]", offset, size);
        ASSERT_ON(size < buffer.size());
        ASSERT_ON(offset == 0);
        return rma.write(offset, buffer);
    }
    Result erase(const StorageAddress &addr) {
        //stub, no implementation
        return mapping.del(addr.addr, addr.size);
    }
	template<typename T>
		requires std::negation_v<std::is_same<T, void>>
	Result read(const StorageAddress &addr, StorageBuffer<T> &buffer){
		return read(addr, buffer.template cast<void>());
	}
    Result read(const StorageAddress &addr, StorageBuffer<> &buffer)  {
        auto [offset, size] = mapping.lookup(addr.addr, addr.size, true);
        LOG_INFO("Storage::read [%lu,%lu]", offset, size);
        ASSERT_ON(size > addr.size);
        ASSERT_ON(addr.size > buffer.allocated());
        ASSERT_ON(offset == 0);
        return rma.read(offset, addr.size, buffer);
    }

    StorageBuffer<> writeb(const StorageAddress &addr) {
        auto [offset, size] = mapping.lookup(addr.addr, addr.size);
        ASSERT_ON(size < addr.size);
        ASSERT_ON(offset == 0);
        return rma.writeb(offset, addr.size);
    }
    StorageBufferRO<> readb(const StorageAddress &addr) {
        auto [offset, size] = mapping.lookup(addr.addr, addr.size, true);
        ASSERT_ON(size < addr.size);
        ASSERT_ON(offset == 0);
        return rma.readb(offset, addr.size);
    }
    Result commit([[maybe_unused]] const StorageBuffer<> &buffer) {
        //stub, no implementation
        return Result::Success;
    }
    Result commit([[maybe_unused]] const StorageBufferRO<> &buffer) {
        //stub, no implementation
        return Result::Success;
    }

    ~SimpleStorage(){
        //serialize to rma
        metadata.va_size = szeimpl::size(mapping) + VirtAddressMapping::EXTRA_SPARE_SPACE;
        metadata.va_offset = mapping.alloc(metadata.va_addr, metadata.va_size);
        StorageBuffer vmap_buf{rma.writeb(metadata.va_offset, metadata.va_size)};
        szeimpl::s(mapping, vmap_buf);

        StorageBuffer metadata_buf{rma.writeb(0, sizeof(FileMetadata))};
        FileMetadata *fm = metadata_buf.template get<FileMetadata>();
        *fm = metadata;
    }

    Result serializeImpl(StorageBuffer<> &buffer) const {
        return szeimpl::s(rma, buffer);
    }
    static SimpleStorage<R> deserializeImpl(const StorageBufferRO<> &buffer, uint32_t id = DataStorageBase::UniqueIDInterface::DEFAULT) {
        return SimpleStorage<R>{R{szeimpl::d<R>(buffer)}, id};
    }
    size_t getSizeImpl() const{
        return szeimpl::size(rma);
    }
};

template <size_t N>
using SimpleFileStorage = SimpleStorage<FileRMA<N> >;

template <size_t N>
using SimpleRamStorage = SimpleStorage<MemoryRMA<N> >;
