#pragma once 

#include <cstdint>

//generic virtual file system
struct StorageAddress{
	uint64_t addr{-1ULL};
	size_t size{0};
};


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
