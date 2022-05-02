#pragma once

#include <map>

#include <libstorage/Utils.hpp>
#include <libstorage/RandomMemoryAccess.hpp>
#include <libstorage/DataStorage.hpp>
#include <libstorage/Serialize.hpp>


/*
	get address -> alloc_unmapped() -> add to unmapped
	stop using -> del() -> remove from unmapped or m
	start using -> lookup() -> alloc() -> add to m, find new range in empty or at the end
	using -> lookup() -> find in m
	expanding address -> expand() -> allocate new mapping alloc() or expand existing m entry
*/
class VirtAddressMapping{
	// two options:
	// <------> file offset -> virtual address mapping, but doesn't give log(N) access in runtime
	// [virtual address, virtual address] -> file offset start - good, but have to manually group
	std::map<uint64_t, std::pair<size_t, size_t>> m; //addr-><offset, size>
	//std::set<std::pair<size_t, size_t>> empty;
	std::map<uint64_t, size_t> unmapped;
	std::multimap<size_t, size_t> empty; //size, offset
	std::pair<uint64_t, size_t> max; //virtaddr, offset

	static constexpr size_t MIN_EMPTYSIZE = 16;
	static constexpr size_t UNMAPPED_OFFSET = std::numeric_limits<size_t>::max();
public:
	static constexpr size_t DEFAULT_SIZE = 1024 * 1024;

	//lookup for mapping addres -> <offset, size>. if not found
	std::pair<size_t, size_t> lookup(uint64_t addr, size_t size, bool is_read_only = false) {
		auto it = m.upper_bound(addr);
		if(it != m.end()){
			--it;
		} else if(m.end() != m.begin()){
			it = std::prev(m.end());
		}
		ASSERT_ON(it == m.end());

		uint64_t va_start = it->first;
		size_t va_size = it->second.second;
		size_t offset = it->second.first;
		size_t addr_offset = addr - va_start;
		ASSERT_ON(va_start > addr);
		ASSERT_ON(va_start + va_size < addr + size);

		if(offset == UNMAPPED_OFFSET){
			ASSERT_ON(is_read_only);
			offset = it->second.first = alloc_offset(addr, size);
		}
		LOG_DEBUG("VirtAddressMapping::lookup {%lx, %lu}->[%lu, %lu]",
			addr, size, offset + addr_offset, va_size - addr_offset);
		LOG_DEBUG("VirtAddressMapping::lookup va_size=%lu addr_offset=%ld va_start=%lx addr=%lx",
			va_size, addr_offset, va_start, addr);
		return std::make_pair(offset + addr_offset,
			va_size - addr_offset);
	}
protected:
	size_t alloc_offset(size_t addr, size_t size){
		auto it = empty.lower_bound(size);
		if(it != empty.end()){
			//check what left
			size_t base_offset = it->second;
			auto newoffset = base_offset + size;
			auto newsize = it->first - size;
			empty.erase(it);
			if(newsize >= MIN_EMPTYSIZE){
				empty.emplace(newsize, newoffset);
			}
			LOG_DEBUG("VirtAddressMapping::alloc_offset {%lx, %lu}->[%lu, %lu]",
				addr, size, base_offset, size);
			return base_offset;
		} else {
			//if nothing, allocate new range at the end
			max.first = addr + size;
			max.second += size;
			LOG_DEBUG("VirtAddressMapping::alloc_offset {%lx, %lu}->[%lu, %lu]",
				addr, size, max.second - size, size);
			return max.second - size;
		}
	}
public:
	//alloc new range
	size_t alloc(uint64_t addr, size_t size){
		size_t offset = alloc_offset(addr, size);
		m.emplace(addr, std::make_pair(offset, size));
		return offset;
	}

	//when we get new addres, or delete range
	Result alloc_unmapped(uint64_t addr, size_t size){
		m.emplace(addr, std::make_pair(UNMAPPED_OFFSET, size));
		return Result::Success;
	}

	Result del(uint64_t addr, size_t size){
		auto it = m.lower_bound(addr);
		if(it == m.end())
			return Result::Failure;
		auto addr_start = it->first;
		auto addr_size = it->second.second;
		auto addr_finish = it->first + addr_size;
		size_t offset = it->second.first;
		ASSERT_ON(addr_start < addr);
		ASSERT_ON(addr_finish > addr + size);

		if(offset == UNMAPPED_OFFSET){
			return Result::Failure;
		}

		if(addr == addr_start) {
			it->second.first = UNMAPPED_OFFSET;
			if(addr_size != size){
				m.emplace(addr_start + size, std::make_pair(offset + size, addr_size - size));
			}
		} else if(addr_finish == addr + size){
			it->second.second -= size;
			m.emplace(addr, std::make_pair(UNMAPPED_OFFSET, size));
		} else {
			//in the middle
			//before [addr_start, addr)
			//after  [addr + size, addr_finish)
			it->second.first = UNMAPPED_OFFSET;
			it->second.second = addr - addr_start - 1;
			m.emplace(addr + size,
				std::make_pair(offset + addr - addr_start + size, addr_finish - addr - size - 1));
		}
		empty.emplace(size, offset); //TODO merge neighbours

		return Result::Success;
	}
	Result expand(StorageAddress address, size_t size){
		//allocate new range with StorageAddress{address.addr + addr.size, size};
		//expand end of file
		if(max.first == address.addr + address.size){
			auto it = m.lower_bound(address.addr);
			//TODO check range
			if(it == m.end()){
				return Result::Failure;
			}
			it->second.second += size;
			max.first += size;
			max.second += size;
			return Result::Success;
		}
		//otherwise allocane new mapping
		alloc(address.addr + address.size, size);
		return Result::Success;
	}

	size_t get_size_extra(size_t extra = 0) const {
		size_t sum = 0;
		sum += sizeof(size_t) + sizeof(decltype(m)::value_type) * (m.size() + extra);
		sum += sizeof(size_t) + sizeof(decltype(empty)::value_type) * (empty.size() + extra);
		sum += sizeof(size_t) + sizeof(decltype(unmapped)::value_type) * (unmapped.size() + extra);
		sum += sizeof(max);
		return sum;
	}

	//only sequential support
	Result serializeImpl(const StorageBuffer<> &buffer) const{
		ASSERT_ON(get_size_extra() > buffer.allocated());
		size_t offset = 0;
		auto buf = buffer.offset_advance(offset, sizeof(decltype(m)::size_type));
		szeimpl::s(m.size(), buf);
		for(auto it: m){
			buf = buffer.offset_advance(offset, sizeof(it));
			szeimpl::s(it, buf);
		}
		buf = buffer.offset_advance(offset, sizeof(decltype(unmapped)::size_type));
		szeimpl::s(unmapped.size(), buf);
		for(auto it: unmapped){
			buf = buffer.offset_advance(offset, sizeof(it));
			szeimpl::s(it, buf);
		}
		buf = buffer.offset_advance(offset, sizeof(decltype(empty)::size_type));
		szeimpl::s(empty.size(), buf);
		for(auto it: empty){
			buf = buffer.offset_advance(offset, sizeof(it));
			szeimpl::s(it, buf);
		}
		buf = buffer.offset_advance(offset, sizeof(max));
		szeimpl::s(max, buf);
		
		return Result::Success;
	}

	static VirtAddressMapping deserializeImpl(const StorageBufferRO<> &buffer) {
		VirtAddressMapping vam;

		size_t offset = 0;
		size_t num;
		auto buf = buffer.offset_advance(offset, sizeof(decltype(m)::size_type));
		num = szeimpl::d<decltype(m)::size_type>(buf);
		for(size_t i = 0; i < num; i++){
			buf = buffer.offset_advance(offset, sizeof(decltype(m)::value_type));
			vam.m.insert(szeimpl::d<decltype(m)::value_type>(buf));
		}
		buf = buffer.offset_advance(offset, sizeof(decltype(unmapped)::size_type));
		num = szeimpl::d<decltype(unmapped)::size_type>(buf);
		for(size_t i = 0; i < num; i++){
			buf = buffer.offset_advance(offset, sizeof(decltype(unmapped)::value_type));
			vam.unmapped.insert(szeimpl::d<decltype(unmapped)::value_type>(buf));
		}
		buf = buffer.offset_advance(offset, sizeof(decltype(empty)::size_type));
		num = szeimpl::d<decltype(empty)::size_type>(buf);
		for(size_t i = 0; i < num; i++){
			buf = buffer.offset_advance(offset, sizeof(decltype(empty)::value_type));
			vam.empty.insert(szeimpl::d<decltype(empty)::value_type>(buf));
		}
		buf = buffer.offset_advance(offset, sizeof(max));
		vam.max = szeimpl::d<decltype(max)>(buf);
		
		return vam;
	}

	size_t getSizeImpl() const {
		return get_size_extra(0);
	}
};

class SimpleStorage: public DataStorage{
	RMAInterface &rma; //e.g. FileRMA{filename}

	//virtual address space => file mapping
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
		const FileMetadata *fm = metadata_buf.get<FileMetadata>();
		if(fm->magic != MAGIC){
			//new file!
			metadata.magic = MAGIC;
			metadata.va_addr = metadata.ra.get_random_address(VirtAddressMapping::DEFAULT_SIZE);
			uint64_t metadata_addr = metadata.ra.get_random_address(sizeof(FileMetadata));
			//first mapping for the beginning of the file
			auto offset = mapping.alloc(metadata_addr, sizeof(FileMetadata));
			ASSERT_ON(offset != 0);
			metadata.static_addr = get_random_address(static_size);
		} else {
			metadata = *fm;
			ASSERT_ON(metadata.static_addr.size < static_size);
			StorageBufferRO vmap_buf{rma.readb(metadata.va_offset, metadata.va_size)};
			mapping = szeimpl::d<decltype(mapping)>(vmap_buf);
			mapping.del(metadata.va_addr, metadata.va_size);
		}
	}
public:
	SimpleStorage(RMAInterface &rma, size_t static_size): rma(rma) {
		init_simple_storage(static_size);
	}
	SimpleStorage(RMAInterface &rma): rma(rma) {
		init_simple_storage(sizeof(StaticHeader));
	}

	StorageAddress get_static_section() override {
		return metadata.static_addr;
	}

	// find random address from definately unused blocks
	StorageAddress get_random_address(size_t size) override {
		uint64_t addr = metadata.ra.get_random_address(size);
		mapping.alloc_unmapped(addr, size);
		return StorageAddress{addr, size};
	}

	// expand existing address range
	StorageAddress expand_address(const StorageAddress &address, size_t size) override {
		mapping.expand(address, size);
		return StorageAddress{address.addr + address.size, size};
	}

	virtual Result write(const StorageAddress &addr, const StorageBuffer<> &buffer) override {
		auto [offset, size] = mapping.lookup(addr.addr, addr.size);
		LOG_INFO("Storage::write [%lu,%lu]", offset, size);
		ASSERT_ON(size < buffer.size());
		ASSERT_ON(offset == 0);
		return rma.write(offset, buffer);
	}
	virtual Result erase(const StorageAddress &addr) override{
		//stub, no implementation
		return mapping.del(addr.addr, addr.size);
	}
	virtual Result read(const StorageAddress &addr, StorageBuffer<> &buffer) override {
		auto [offset, size] = mapping.lookup(addr.addr, addr.size, true);
		LOG_INFO("Storage::read [%lu,%lu]", offset, size);
		ASSERT_ON(size > addr.size);
		ASSERT_ON(addr.size > buffer.allocated());
		ASSERT_ON(offset == 0);
		return rma.read(offset, addr.size, buffer);
	}

	virtual StorageBuffer<> writeb(const StorageAddress &addr) override{
		auto [offset, size] = mapping.lookup(addr.addr, addr.size);
		ASSERT_ON(size < addr.size);
		ASSERT_ON(offset == 0);
		return rma.writeb(offset, addr.size);
	}
	virtual StorageBufferRO<> readb(const StorageAddress &addr) override{
		auto [offset, size] = mapping.lookup(addr.addr, addr.size, true);
		ASSERT_ON(size > addr.size);
		ASSERT_ON(offset == 0);
		return rma.readb(offset, addr.size);
	}
	virtual Result commit(const StorageBuffer<> &buffer) override{
		//stub, no implementation
		return Result::Success;
	}
	virtual Result commit(const StorageBufferRO<> &buffer) override{
		//stub, no implementation
		return Result::Success;
	}

	virtual Stat stat(const StorageAddress &addr) override{
		//stub, no implementation
		return Stat{};
	}

	virtual ~SimpleStorage(){
		//serialize to rma
		metadata.va_size = mapping.get_size_extra(1);
		metadata.va_offset = mapping.alloc(metadata.va_addr, metadata.va_size);
		StorageBuffer vmap_buf{rma.writeb(metadata.va_offset, metadata.va_size)};
		szeimpl::s(mapping, vmap_buf);

		StorageBuffer metadata_buf{rma.writeb(0, sizeof(FileMetadata))};
		FileMetadata *fm = metadata_buf.get<FileMetadata>();
		*fm = metadata;
	}
};
