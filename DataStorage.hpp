#pragma once

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
	StorageBuffer buf;
	size_t size;
	//TODO things to give direct access
}

//generic virtual file system
struct StorageAddress{
	uint64_t offset;
	size_t size;
};

class DataStorage{
public:
	struct Stat{
		//TODO
	};
	virtual StorageRawBuffer write_raw_start(const StorageAddress &addr) = 0;
	virtual Result write_raw_finish(const StorageRawBuffer &raw_buffer) = 0;
	virtual Result write(const StorageAddress &addr, const StorageBuffer &buffer) = 0;

	virtual Result erase(const StorageAddress &addr) = 0;

	virtual StorageRawBuffer read_raw_start(const StorageAddress &addr) = 0;
	virtual Result read_raw_finish(const StorageRawBuffer &raw_buffer) = 0;
	virtual Result read(const StorageAddress &addr, const StorageBuffer &buffer) = 0;

	virtual Stat stat(const StorageAddress &addr) = 0;
};

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
