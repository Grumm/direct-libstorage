#pragma once 

#include <cstdint>
#include <type_traits>

#include <storage/Utils.hpp>

//generic virtual file system

template<typename T>
class StorageBufferRO;

template<typename T>
class StorageBuffer;

struct StorageAddress{
	static constexpr uint64_t NULL_ADDR = -1ULL;
	uint64_t addr{NULL_ADDR};
	size_t size{0};

	bool is_null() const { return addr == NULL_ADDR; }
	void reset() { addr = NULL_ADDR; size = 0; }

	StorageAddress subrange(size_t offset, size_t newsize) const {
		ASSERT_ON(size < offset + newsize);
		return StorageAddress{addr + offset, newsize};
	}
};

template<typename T = void>
class StorageBuffer{
	T *data{nullptr};
	size_t data_size{0}; //how much data we have
	size_t data_allocated{0}; //max size of the buffer
public:
	StorageBuffer() {}
	StorageBuffer(T *data, size_t alloc_): data(data), data_allocated(alloc_) {}
	StorageBuffer(T *data, size_t size_, size_t alloc_):
		data(data), data_size(size_), data_allocated(alloc_) {}
	StorageBuffer(StorageBuffer &&buf) = default;

	template<typename U = T>
	constexpr StorageBuffer(const StorageBuffer<U> &buf):
		data(buf.template get<T>()), data_size(buf.size()), data_allocated(buf.allocated()) {}
	constexpr StorageBuffer(const StorageBuffer<T> &buf):
		data(buf.get()), data_size(buf.size()), data_allocated(buf.allocated()) {}

	constexpr StorageBuffer &operator=(const StorageBuffer &buf){
		data = buf.get();
		data_size = buf.size();
		data_allocated = buf.allocated();

		return *this;
	}
	template<typename U = T>
	constexpr StorageBuffer &operator=(const StorageBuffer<U> &buf){
		data = buf.template get<T>();
		data_size = buf.size();
		data_allocated = buf.allocated();

		return *this;
	}
	operator StorageBuffer<void>() const { return this->template cast<void>(); }

	template<typename U = T>
	StorageBuffer<U> &cast() {
		return *reinterpret_cast<StorageBuffer<U> *>(this);
	}
	template<typename U = T>
	const StorageBuffer<U> &cast() const{
		return *reinterpret_cast<const StorageBuffer<U> *>(this);
	}

	template<typename U = T>
	U *get(size_t offset_ = 0) const{
		return reinterpret_cast<U *>(PTR_OFFSET(data, offset_));
	}

	size_t size() const{
		return data_size;
	}
	size_t allocated() const{
		return data_allocated;
	}

	void reset(){
		data_size = 0;
	}

	//put more data at the end, return new sub-buffer
	StorageBuffer advance(size_t _size) {
		ASSERT_ON(size() + _size > allocated());

		StorageBuffer buf{offset(size(), _size)};

		PTR_OFFSET(data, _size);
		data_size += _size;

		return buf;
	}

	//returns new buffer of _size by _offset
	StorageBuffer offset(size_t _offset, size_t _size) const{
		ASSERT_ON(_offset + _size > allocated());
		return StorageBuffer{PTR_OFFSET(data, _offset), _size, _size};
	}

	//returns new buffer of _size by _offset, advances the _offset after that
	//used for iteration over buffer
	StorageBuffer offset_advance(size_t &_offset, size_t _size) const{
		ASSERT_ON(_offset + _size > allocated());
		size_t __offset = _offset;
		_offset += _size;

		return offset(__offset, _size);
	}
	//advance offset, return the rest of buffer
	StorageBuffer advance_offset(size_t &_offset, size_t _size) const{
		ASSERT_ON(_offset + _size > allocated());
		_offset += _size;

		return offset(_offset, allocated() - _offset);
	}
};

template<typename T = void>
class StorageBufferRO{
	const T *data{nullptr};
	size_t data_size{0}; //how much data we have, can only decrease
public:
	StorageBufferRO(const T *data, size_t size_): data(data), data_size(size_) {}
	template<typename U = T>
	StorageBufferRO(StorageBuffer<U> &buf):
		data(buf.template get<T>()), data_size(buf.allocated()) {}
	template<typename U = T>
	StorageBufferRO(const StorageBufferRO<U> &other):
		data(other.template get<T>()), data_size(other.size()) {}
	StorageBufferRO(StorageBufferRO &&buf) = default;
	StorageBufferRO(const StorageBufferRO &buf) = default;

	operator StorageBufferRO<void>() const { return this->template cast<void>(); }


	constexpr StorageBufferRO &operator=(const StorageBufferRO &buf){
		data = buf.get();
		data_size = buf.size();

		return *this;
	}
	template<typename U = T>
	constexpr StorageBufferRO &operator=(const StorageBufferRO<U> &buf){
		data = buf.template get<T>();
		data_size = buf.size();

		return *this;
	}
	template<typename U = T>
	StorageBufferRO<U> &cast() {
		return *reinterpret_cast<StorageBufferRO<U> *>(this);
	}
	template<typename U = T>
	const StorageBufferRO<U> &cast() const{
		return *reinterpret_cast<const StorageBufferRO<U> *>(this);
	}

	template<typename U = T>
	const U *get(size_t offset_ = 0) const{
		return reinterpret_cast<const U *>(PTR_OFFSET(data, offset_));
	}

	void shrink(size_t delta){
		ASSERT_ON(data_size < delta);
		data_size -= delta;
	}
	size_t size() const{
		return data_size;
	}

	//returns new buffer of _size by _offset
	StorageBufferRO offset(size_t _offset, size_t _size) const{
		ASSERT_ON(_offset + _size > size());
		return StorageBufferRO{PTR_OFFSET(data, _offset), _size};
	}

	//returns new buffer of _size by _offset, advances the _offset after that
	//used for iteration over buffer
	StorageBufferRO offset_advance(size_t &_offset, size_t _size) const{
		ASSERT_ON(_offset + _size > size());
		size_t __offset = _offset;
		_offset += _size;

		return offset(__offset, _size);
	}
	//advance offset, return the rest of buffer
	StorageBufferRO advance_offset(size_t &_offset, size_t _size) const{
		ASSERT_ON(_offset + _size > size());
		_offset += _size;

		return offset(_offset, size() - _offset);
	}
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

enum class AllocationPattern{
    Sequential,
    Random,
    Ranges,
    Default = Sequential,
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
