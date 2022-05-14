#pragma once

#include <functional>
#include <cstring>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <storage/Utils.hpp>
#include <storage/StorageUtils.hpp>
#include <storage/SerializeImpl.hpp>

#ifndef MADV_HUGEPAGE
//#warning "Hugepages not supported. Using compatible mode"
#define MADV_HUGEPAGE 14
#endif

#ifndef MAP_AUTOGROW
#define MAP_AUTOGROW 0x8000
#endif

template<typename T>
concept CRMAImpl = requires(T &t, size_t offset, size_t size, StorageBuffer<> &buffer){
    { t.open() } -> std::same_as<Result>;
    { t.close() } -> std::same_as<Result>;
    { t.write(offset, buffer) } -> std::same_as<Result>;
    { t.read(offset, size, buffer) } -> std::same_as<Result>;

    { t.writeb(offset, size) } -> std::same_as<StorageBuffer<>>;
    { t.readb(offset, size) } -> std::same_as<StorageBufferRO<>>;
};
template<typename T>
concept CRMA = CRMAImpl<T> && CSerializableImpl<T>;

template<size_t MAX_FILESIZE_OFFSET = 32> //4GiB
class FileRMA{
    std::string filename;
    bool is_open{false};
    int fd{-1};
    void *membase;
    void *membase_readonly;
    static constexpr size_t MAX_FILESIZE = (1UL << MAX_FILESIZE_OFFSET);
    //TODO call msync(membase, MAX_FILESIZE, MS_ASYNC) periodically
    //TODO use madvise(membase, MAX_FILESIZE, ) when needed https://linux.die.net/man/2/madvise
public:
    FileRMA(const std::string &filename): filename(filename) { open(); }
    Result open(){
        fd = ::open(filename.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        if(fd == -1){
            int e = errno;
            LOG_ERROR("Failed to open file %s with %x", filename.c_str(), e);
            return Result::Failure;
        }
        //MAP_PRIVATE to enable Huge Pages
        membase = mmap(0, MAX_FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_AUTOGROW, fd, 0);
        if(membase == MAP_FAILED){
            int e = errno;
            LOG_ERROR("Failed to mmap membase with %x", e);
            ::close(fd);
            return Result::Failure;
        }
        int advise = madvise(membase, MAX_FILESIZE, MADV_HUGEPAGE);
        if(advise != 0){
            int e = errno;
            LOG_INFO("Advise failed on membase with %x", e);
            if(e == EAGAIN){
                //TODO loop N times
            }
        }
        membase_readonly = mmap(0, MAX_FILESIZE, PROT_READ, MAP_SHARED | MAP_AUTOGROW, fd, 0);
        if(membase_readonly == MAP_FAILED){
            int e = errno;
            LOG_ERROR("Failed to mmap membase_readonly with %x", e);
            ::munmap(membase, MAX_FILESIZE);
            ::close(fd);
            return Result::Failure;
        }
        advise = madvise(membase_readonly, MAX_FILESIZE, MADV_HUGEPAGE);
        if(advise != 0){
            int e = errno;
            LOG_INFO("Advise failed on membase_readonly with %x", e);
            if(e == EAGAIN){
                //TODO loop N times
            }
        }
        is_open = true;
        return Result::Success;
    }
    Result close(){
        if(is_open){
            is_open = false;
            ::munmap(membase_readonly, MAX_FILESIZE);
            ::munmap(membase, MAX_FILESIZE);
            ::close(fd);
        }
        return Result::Success;
    }
    template<typename T>
        requires std::negation_v<std::is_same<T, void>>
    Result read(size_t offset, size_t size, StorageBuffer<T> &buffer){
        return read(offset, size, buffer.template cast<void>());
    }
    template<typename T>
        requires std::negation_v<std::is_same<T, void>>
    Result write(size_t offset, size_t size, StorageBuffer<T> &buffer){
        return write(offset, size, buffer.template cast<void>());
    }
    Result write(size_t offset, const StorageBuffer<> &buffer){
        ASSERT_ON(!is_open);
        LOG_INFO("RMA:write [%lu,%lu]", offset, buffer.size());
        memcpy(PTR_OFFSET(membase, offset), buffer.get(), buffer.size());
        return Result::Success;
    }
    Result read(size_t offset, size_t size, StorageBuffer<> &buffer){
        ASSERT_ON(!is_open);
        ASSERT_ON(size > buffer.allocated());
        LOG_INFO("RMA:read [%lu,%lu]", offset, size);
        memcpy(buffer.get(), PTR_OFFSET(membase, offset), size);
        buffer.reset();
        buffer.advance(size);
        return Result::Success;
    }
    StorageBuffer<> writeb(size_t offset, size_t size){
        ASSERT_ON(!is_open);
        return StorageBuffer{PTR_OFFSET(membase, offset), size, size};
    }
    StorageBufferRO<> readb(size_t offset, size_t size){
        ASSERT_ON(!is_open);
        return StorageBufferRO{PTR_OFFSET(membase_readonly, offset), size};
    }
    ~FileRMA(){
        if (is_open){
            close();
        }
    }

    Result serializeImpl(StorageBuffer<> &buffer) const{
        ASSERT_ON_MSG(buffer.size() < getSizeImpl(), "Buffer size too small");
        Result res = Result::Success;

        size_t offset = 0;
        StorageBuffer buf = buffer.offset_advance(offset, szeimpl::size(MAX_FILESIZE_OFFSET));
        res = szeimpl::s(MAX_FILESIZE_OFFSET, buf);
        buf = buffer.offset_advance(offset, szeimpl::size(filename));
        res = szeimpl::s(filename, buf);
    
        return res;
    }

    static FileRMA<MAX_FILESIZE_OFFSET> deserializeImpl(const StorageBufferRO<> &buffer) {
        size_t offset = 0;
        StorageBufferRO buf = buffer;

        auto max_filesize_offset = szeimpl::d<decltype(MAX_FILESIZE_OFFSET)>(buf);
        buf = buffer.advance_offset(offset, szeimpl::size(max_filesize_offset));
        ASSERT_ON(max_filesize_offset != MAX_FILESIZE_OFFSET);

        return FileRMA<MAX_FILESIZE_OFFSET>{szeimpl::d<decltype(filename)>(buf)};
    }
    size_t getSizeImpl() const{
        return szeimpl::size(MAX_FILESIZE_OFFSET) + szeimpl::size(filename);
    }
};

template<size_t MAX_MEMSIZE_OFFSET = 32> //4GiB
class MemoryRMA{
    bool is_open{false};
    void *membase;
    static constexpr size_t MAX_MEMSIZE = (1UL << MAX_MEMSIZE_OFFSET);
public:
    MemoryRMA() { open(); }
    Result open(){
        membase = mmap(0, MAX_MEMSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_AUTOGROW, -1, 0);
        if(membase == MAP_FAILED){
            int e = errno;
            LOG_ERROR("Failed to mmap membase with %x", e);
            return Result::Failure;
        }
        int advise = madvise(membase, MAX_MEMSIZE, MADV_HUGEPAGE);
        if(advise != 0){
            int e = errno;
            LOG_INFO("Advise failed on membase with %x", e);
            if(e == EAGAIN){
                //TODO loop N times
            }
        }
        is_open = true;
        return Result::Success;
    }
    Result close(){
        if(is_open){
            is_open = false;
            munmap(membase, MAX_MEMSIZE);
        }
        return Result::Success;
    }
    template<typename T>
        requires std::negation_v<std::is_same<T, void>>
    Result read(size_t offset, size_t size, StorageBuffer<T> &buffer){
        return read(offset, size, buffer.template cast<void>());
    }
    template<typename T>
        requires std::negation_v<std::is_same<T, void>>
    Result write(size_t offset, size_t size, StorageBuffer<T> &buffer){
        return write(offset, size, buffer.template cast<void>());
    }
    Result write(size_t offset, const StorageBuffer<> &buffer){
        ASSERT_ON(!is_open);
        memcpy(PTR_OFFSET(membase, offset), buffer.get(), buffer.size());
        return Result::Success;
    }
    Result read(size_t offset, size_t size, StorageBuffer<> &buffer){
        ASSERT_ON(!is_open);
        ASSERT_ON(size > buffer.allocated());
        memcpy(buffer.get(), PTR_OFFSET(membase, offset), size);
        buffer.reset();
        buffer.advance(size);
        return Result::Success;
    }
    StorageBuffer<> writeb(size_t offset, size_t size){
        ASSERT_ON(!is_open);
        return StorageBuffer{PTR_OFFSET(membase, offset), size, size};
    }
    StorageBufferRO<> readb(size_t offset, size_t size){
        ASSERT_ON(!is_open);
        return StorageBufferRO{PTR_OFFSET(membase, offset), size};
    }
    ~MemoryRMA(){
        if (is_open){
            close();
        }
    }

    Result serializeImpl(StorageBuffer<> &buffer) const{
        ASSERT_ON_MSG(buffer.size() < getSizeImpl(), "Buffer size too small");
        return szeimpl::s(MAX_MEMSIZE_OFFSET, buffer);
    }

    static MemoryRMA<MAX_MEMSIZE_OFFSET> deserializeImpl(const StorageBufferRO<> &buffer) {
        size_t offset = 0;
        StorageBufferRO buf = buffer;
    
        auto max_memsize_offset = szeimpl::d<decltype(MAX_MEMSIZE_OFFSET)>(buf);
        ASSERT_ON(max_memsize_offset != MAX_MEMSIZE_OFFSET);

        return MemoryRMA<MAX_MEMSIZE_OFFSET>{};
    }
    size_t getSizeImpl() const{
        return szeimpl::size(MAX_MEMSIZE_OFFSET);
    }
};

//TODO for memory hierarchy
#if 0
class NetworkRMAServer{
public:
    NetworkRMAServer(..network info..){}
    Result start(){
        //RPC server
    }
    Result stop(){
        ...
    }
};
class NetworkRMA: public RMAInterface{
    bool is_open{false};
    void *membase;
    static constexpr MAX_MEMSIZE = 1024 * 1024 * 1024 * 1024 * 1024; //1 exabyte
public:
    NetworkRMA(..network info...) { open(); }
    virtual Result open() override{
        membase = mmap(0, MAX_MEMSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if(membase == MAP_FAILED){
            int e = errno;
            LOG_ERROR("Failed to mmap membase with %x", e);
            return Result::Failure;
        }
        int advise = madvise(membase, MAX_MEMSIZE, MADV_HUGEPAGE);
        if(advise != 0){
            int e = errno;
            LOG_INFO("Advise failed on membase with %x", e);
            if(e == EAGAIN){
                //TODO loop N times
            }
        }
        is_open = true;
        return Result::Success;
    }
    virtual Result close() override{
        if(is_open){
            is_open = false;
            munmap(membase, MAX_FILESIZE);
        }
    }
    virtual Result write(size_t offset, const StorageBuffer<> &buffer) override{
        ASSERT_ON(!is_open);
        memcpy(membase + offset, buffer.get(), buffer.size);
        return Result::Success;
    }
    virtual Result read(size_t offset, size_t size, StorageBuffer<> &buffer) override{
        ASSERT_ON(!is_open);
        ASSERT_ON(size > buffer.allocated());
        memcpy(buffer.get(), membase + offset, size);
        buffer.size = size;
        return Result::Success;
    }
    virtual StorageBuffer writeb(size_t offset, size_t size) override {
        ASSERT_ON(!is_open);
        return StorageBuffer{membase + offset, size, size};
    }
    virtual StorageBufferRO readb(size_t offset, size_t size) override {
        ASSERT_ON(!is_open);
        return StorageBufferRO{membase + offset, size, size};
    }
    virtual ~NetworkRMA(){
        if (is_open){
            close();
        }
    }
};
#endif