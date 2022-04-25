#pragma once
#include "DataStorage.hpp"


struct ObjectAddress{
	uint64_t storageId:32;
	uint64_t index:32;
};

struct ObjectSequenceAddress{
	uint32_t storageId;
	std::vector<uint32_t> indices;
};


/****************************************************/

//interface for all object storages - db, sequence etc. T - Serializable
template<class T, class DS> //typename <template> ?
class ObjectStorage{
public:
	StorageRawBuffer addRawObjectStart();
	ObjectAddress addRawObjectFinish(const StorageRawBuffer &raw_buffer);

	StorageRawBuffer getRawObject(const ObjectAddress &addr);
	Result putRawObject(const StorageRawBuffer &raw_buffer);

	/* return ref. object owned by storage */
	std::unique_ptr<const T> getObject(const ObjectAddress &addr);
	Result releaseObject(std::unique_ptr<const T> obj);
	std::unique_ptr<T> getMutableObject(const ObjectAddress &addr);
};

//extending object storage to handle collections
template<class T>
class SequenceStorage:
	public ObjectStorage<T>{
public:
	getObjects()
};

/****************************************************/
