#pragma once

#include <type_traits>
#include <functional>

#include <storage/Utils.hpp>
#include <storage/StorageUtils.hpp>
#include <storage/SerializeImpl.hpp>
#include <storage/UniqueID.hpp>

class DataStorage: public UniqueIDInstance {
public:
	struct Stat{
		//TODO
	};
	template<typename T>
		requires std::negation_v<std::is_same<T, void>>
	Result read(const StorageAddress &addr, StorageBuffer<T> &buffer){
		return read(addr, buffer.template cast<void>());
	}
	template<typename T>
		requires std::negation_v<std::is_same<T, void>>
	Result write(const StorageAddress &addr, const StorageBuffer<T> &buffer){
		return write(addr, buffer.template cast<void>());
	}
	template<typename T>
		requires std::negation_v<std::is_same<T, void>>
	Result commit(const StorageBuffer<T> &buffer){
		return commit(buffer.template cast<void>());
	}
	template<typename T>
		requires std::negation_v<std::is_same<T, void>>
	Result commit(const StorageBufferRO<T> &buffer){
		return commit(buffer.template cast<void>());
	}
	template<typename T>
		requires std::negation_v<std::is_same<T, void>>
	StorageBuffer<T> writeb(const StorageAddress &addr){
		return writeb(addr).template cast<T>();
	}
	template<typename T>
		requires std::negation_v<std::is_same<T, void>>
	StorageBufferRO<T> readb(const StorageAddress &addr){
		return writeb(addr).template cast<T>();
	}

	virtual StorageAddress get_static_section() = 0;
	virtual StorageAddress get_random_address(size_t size) = 0;
	virtual StorageAddress expand_address(const StorageAddress &address, size_t size) = 0;

	virtual Result write(const StorageAddress &addr, const StorageBuffer<> &buffer) = 0;
	virtual Result erase(const StorageAddress &addr) = 0;
	virtual Result read(const StorageAddress &addr, StorageBuffer<> &buffer) = 0;

	virtual StorageBuffer<> writeb(const StorageAddress &addr) = 0;
	virtual StorageBufferRO<> readb(const StorageAddress &addr) = 0;
	virtual Result commit(const StorageBuffer<> &buffer) = 0; //TODO hint where data has been changed?
	virtual Result commit(const StorageBufferRO<> &buffer) = 0;

	virtual Stat stat(const StorageAddress &addr) = 0;

	DataStorage(): UniqueIDInstance(GenerateGlobalUniqueID()) {}
	virtual ~DataStorage(){}

    virtual Result serializeImpl(StorageBuffer<> &buffer) const = 0;
	template<typename T>
    static T deserializeImpl(const StorageBufferRO<> &buffer) {
		throw std::bad_function_call("Not implemented. Deserializing abstract class");
	}
    virtual size_t getSizeImpl() const = 0;
};

//TODO reorganize
static inline Result initialize_zero(DataStorage &storage, const StorageAddress &addr){
	auto buf = storage.writeb(addr);
	memset(buf.get(), 0, buf.allocated());
	return storage.commit(buf);
}

struct StaticHeader{
	uint64_t magic;
	StorageAddress address;
};

/*
For DataStorage we need some way to store some static information.
Q1: way to identify static address:
- defined in DataStorage constant, fixed address
- queried from DataStorage object

Q2: how to allocate
- allocated on storage creation automatically
- allocated on demand

Q3: what we store there:
- whole section, which maps multiple: string key -> storage address
- one storage address
- unspecified

A: Q1 - don't want to overcomplicate DataStorage interface, fixed const address
A: Q2 - to simplify storage object creation, make it a interface requirement?
A: Q3 - map would be nice, but 

Three options:
- fixed constant address, created automatically, one storage address
- queried from datastorage, allocated on demand, key->address map
- queried from datastorage, created automatically, unspecified [V]
*/




#if 0

//vfs implementation for storing in files

class FileStorage: public DataStorage, public RankStorage{

public:
};

//TODO vfs implementation for multi-level storage(MLS)

//interface to all classes that want to be in MLS
class RankStorage{

public:
};

//template<...?>
class MultiRankStorage: public DataStorage{

public:
};

//TODO vfs implementation for storing in ram

class DRAMStorage: public DataStorage, public RankStorage{

public:
};
#endif

#if 0

//Addressing raw data(DataStorage)
using ExampleMRStorage = MultiRankStorage<MemoryStorage, SSDStorage, DiskStorage, NetworkStorage>;

template <typename T...>
class MultiRankStorage{
public:
	
};

class DataStorage{
public:
	struct Stat{
		//TODO
	};
	virtual StorageRawBuffer write_raw_start(const StorageAddress &addr) = 0;
	virtual Result write_raw_finish(const StorageAddress &addr, const StorageRawBuffer &raw_buffer) = 0;

	virtual Result write(const StorageAddress &addr, const StorageBuffer &buffer) = 0;
	virtual Result erase(const StorageAddress &addr) = 0;

	virtual StorageRawBuffer read_raw_start(const StorageAddress &addr) = 0;
	virtual Result read_raw_finish(const StorageAddress &addr) = 0;
	virtual Result read(const StorageAddress &addr, const StorageBuffer &buffer) = 0;

	virtual Stat stat(const StorageAddress &addr) = 0;
};

class RankStorage{ //TODO maintain fragmented address space mapping to memory or file
public:
	virtual isAvailable(size_t size) = 0;
	virtual StorageRawBuffer getBuffer(const StorageAddress &addr) = 0;
};

class RankStorageManager{ //TODO maintain LRU or usage patterns
public:
	virtual rotate() = 0; //?
};

template <typename R1, typename R2>
class RankStorage2: public DataStorage{ //TODO maintain address space mapping to R1 and to R2, and synchronized flag
public:
	virtual StorageRawBuffer write_raw_start(const StorageAddress &addr) override;
	virtual Result write_raw_finish(const StorageAddress &addr, const StorageRawBuffer &raw_buffer) override;

	virtual Result write(const StorageAddress &addr, const StorageBuffer &buffer) override {

	}
	virtual Result erase(const StorageAddress &addr) override;

	virtual StorageRawBuffer read_raw_start(const StorageAddress &addr) override;
	virtual Result read_raw_finish(const StorageAddress &addr) override;
	virtual Result read(const StorageAddress &addr, const StorageBuffer &buffer) override;

	virtual Stat stat(const StorageAddress &addr) override;
};

template <typename T>
class DataStorage{
public:

};


class enum Result{
	Success,
	Failure,

	bool isSuccess() const{
		return *this == Success;
	}
};

struct StorageBuffer{
	void *data;
	size_t size;
};

struct StorageRawBuffer{
	//TODO things to give direct access
}

//generic virtual file system
struct StorageAddress{
	uint64_t offset;
	size_t size;
};

//Addressing raw bytes
using ExampleDataStorage = DataStorage<ExampleMRStorage>;
#endif