#pragma once

#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <storage/Utils.hpp>
#include <storage/StorageUtils.hpp>

#ifndef MADV_HUGEPAGE
#warning "Hugepages not supported. Using compatible mode"
#define MADV_HUGEPAGE 14
#endif

#ifndef MAP_AUTOGROW
#define MAP_AUTOGROW 0x8000
#endif

class RMAInterface{
public:
	template<typename T>
		requires std::negation_v<std::is_same<T, void>>
	Result read(size_t offset, size_t size, StorageBuffer<T> &buffer){
		return read(offset, size, buffer.template cast<void>());
	}

	virtual Result open() = 0;
	virtual Result close() = 0;
	virtual Result write(size_t offset, const StorageBuffer<> &buffer) = 0;
	virtual Result read(size_t offset, size_t size, StorageBuffer<> &buffer) = 0; //read to buffer

	virtual StorageBuffer<> writeb(size_t offset, size_t size) = 0; //return buffer for read-write
	virtual StorageBufferRO<> readb(size_t offset, size_t size) = 0; //return read-only buffer
};

template<size_t MAX_FILESIZE_OFFSET = 32> //4GiB
class FileRMA: public RMAInterface{
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
	virtual Result open() override{
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
	virtual Result close() override{
		if(is_open){
			is_open = false;
			::munmap(membase_readonly, MAX_FILESIZE);
			::munmap(membase, MAX_FILESIZE);
			::close(fd);
		}
		return Result::Success;
	}
	virtual Result write(size_t offset, const StorageBuffer<> &buffer) override{
		ASSERT_ON(!is_open);
		LOG_INFO("RMA:write [%lu,%lu]", offset, buffer.size());
		memcpy(PTR_OFFSET(membase, offset), buffer.get(), buffer.size());
		return Result::Success;
	}
	virtual Result read(size_t offset, size_t size, StorageBuffer<> &buffer) override{
		ASSERT_ON(!is_open);
		ASSERT_ON(size > buffer.allocated());
		LOG_INFO("RMA:read [%lu,%lu]", offset, size);
		memcpy(buffer.get(), PTR_OFFSET(membase, offset), size);
		buffer.reset();
		buffer.advance(size);
		return Result::Success;
	}
	virtual StorageBuffer<> writeb(size_t offset, size_t size) override {
		ASSERT_ON(!is_open);
		return StorageBuffer{PTR_OFFSET(membase, offset), size, size};
	}
	virtual StorageBufferRO<> readb(size_t offset, size_t size) override {
		ASSERT_ON(!is_open);
		return StorageBufferRO{PTR_OFFSET(membase_readonly, offset), size};
	}
	virtual ~FileRMA(){
		if (is_open){
			close();
		}
	}
};

template<size_t MAX_MEMSIZE_OFFSET = 32> //4GiB
class MemoryRMA: public RMAInterface{
	bool is_open{false};
	void *membase;
	static constexpr size_t MAX_MEMSIZE = (1UL << MAX_MEMSIZE_OFFSET);
public:
	MemoryRMA() { open(); }
	virtual Result open() override{
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
	virtual Result close() override{
		if(is_open){
			is_open = false;
			munmap(membase, MAX_MEMSIZE);
		}
		return Result::Success;
	}
	virtual Result write(size_t offset, const StorageBuffer<> &buffer) override{
		ASSERT_ON(!is_open);
		memcpy(PTR_OFFSET(membase, offset), buffer.get(), buffer.size());
		return Result::Success;
	}
	virtual Result read(size_t offset, size_t size, StorageBuffer<> &buffer) override{
		ASSERT_ON(!is_open);
		ASSERT_ON(size > buffer.allocated());
		memcpy(buffer.get(), PTR_OFFSET(membase, offset), size);
		buffer.reset();
		buffer.advance(size);
		return Result::Success;
	}
	virtual StorageBuffer<> writeb(size_t offset, size_t size) override {
		ASSERT_ON(!is_open);
		return StorageBuffer{PTR_OFFSET(membase, offset), size, size};
	}
	virtual StorageBufferRO<> readb(size_t offset, size_t size) override {
		ASSERT_ON(!is_open);
		return StorageBufferRO{PTR_OFFSET(membase, offset), size};
	}
	virtual ~MemoryRMA(){
		if (is_open){
			close();
		}
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