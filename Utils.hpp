#pragma once

#include <cstdio>
#include <string>
#include <filesystem>
#include <source_location>
#include <exception>

#define LOG(LEVEL, str, ...) do{ \
		auto location = std::source_location::current(); \
		fprintf(stderr, "[%s %s %s:%d '%s'] " str "\n", \
			#LEVEL, __TIME__, std::filesystem::path(location.file_name()).filename().c_str(), \
			location.line(), location.function_name(), ##__VA_ARGS__); \
	}while(0)
#define LOG_INFO(str, ...) LOG(INFO, str, ##__VA_ARGS__)
#define LOG_WARNING(str, ...) LOG(WARNING, str, ##__VA_ARGS__)
#define LOG_ERROR(str, ...) LOG(ERROR, str, ##__VA_ARGS__)
#define LOG_ASSERT(str, ...) LOG(ASSERT, str, ##__VA_ARGS__)

#ifdef NDEBUG
#define ASSERT_ON(cond)
#else
#define ASSERT_ON(cond) do{ \
		if(cond) [[unlikely]]{ \
			LOG_ASSERT("Assert " #cond); \
			throw std::logic_error("Assert " #cond); \
		} \
	}while(0)
#endif

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG == 0
#define LOG_DEBUG(str, ...)
#else
#define LOG_DEBUG(str, ...) LOG(DEBUG, str, ##__VA_ARGS__)
#endif

enum class Result{
	Success,
	Failure,
};

template <typename T>
const T *PTR_OFFSET(const T *base, size_t offset){
	return static_cast<const T *>(static_cast<const char *>(base) + offset);
}
template <typename T>
T *PTR_OFFSET(T *base, size_t offset){
	return static_cast<T *>(static_cast<char *>(base) + offset);
}

//TODO move to StorageUtils.hpp
struct StorageBuffer{
	void *data{nullptr};
	size_t size{0}; //how much data we have
	size_t alloc{0}; //max size of the buffer
};

struct StorageBufferRO{
	const void *data;
	const size_t size; //how much data we have
	const size_t alloc; //max size of the buffer
	//XXX maybe actual data offset?

	StorageBufferRO(StorageBuffer &&buf):
		data(buf.data), size(buf.size), alloc(buf.alloc) {}
	StorageBufferRO(const StorageBufferRO &other):
		data(other.data), size(other.size), alloc(other.alloc) {}
	StorageBufferRO(void *data, size_t size, size_t alloc): data(data), size(size), alloc(alloc) {}
};


//TODO move to StorageUtils or DataStorage.hpp
template<size_t BITS_OFFSET = 36>
class RandomAddressRange{
	uint64_t high_bits;
public:
	RandomAddressRange(): high_bits(0) {}
	uint64_t get_random_address(size_t size){
		size_t max_offset = __builtin_clz(size);
		if(max_offset > BITS_OFFSET){
			uint64_t diff_offset = max_offset - BITS_OFFSET;
			high_bits += (1 << diff_offset);
		} else {
			high_bits++;
		}
		uint64_t address = (high_bits << BITS_OFFSET);
		return address;
	}
};

#if 0
class StorageRawBuffer{
	DataStorage &storage;
	size_t size;
public:
	std::unique_ptr<StorageBuffer> buffer;
	void get_size() const {
		return size;
	}
	void set_size(size_t new_size){
		if (new_size > buffer->size){
			throw std::out_of_range;
		}
		size = new_size;
	}

	StorageRawBuffer(DataStorage &storage, size_t size, StorageBuffer &buffer):
		storage(storage), size(0), buffer(buffer) {}
	~StorageRawBuffer(){
		storage.commit(std::move(*buffer), size);
	}
};
#endif