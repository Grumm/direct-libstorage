#pragma once
#include "DataStorage.hpp"


class SerializableImplExample:
	public Serializable<SerializableImplExample>{
public:
	Result serializeImpl(const StorageRawBuffer &raw_buffer);
	static SerializableImplExample deserializeImpl(const StorageRawBuffer &raw_buffer);
	constexpr size_t getSizeImpl();
};

/****************************************************/

struct ObjectAddress{
	uint64_t storageId:32;
	uint64_t index:32;
};

struct ObjectSequenceAddress{
	uint32_t storageId;
	std::vector<uint32_t> indices;
};

/****************************************************/

template<class T>
class Serializable{
public:
	ObjectAddress serialize(ObjectStorage<T> &storage){
		StorageRawBuffer raw_buffer = storage.addRawObjectStart();
		auto result = static_cast<T*>(this)->serializeImpl(raw_buffer);
		return storage.addRawObjectFinish(raw_buffer);
	}
	static T deserialize(ObjectStorage<T> &storage, const ObjectAddress &addr){
		StorageRawBuffer raw_buffer = storage.getRawObject(addr);
		T &&obj = T::deserializeImpl(raw_buffer);
		storage.putRawObject(raw_buffer);
		return obj;
	}
	static constexpr size_t getSize(){
		return T::getSizeImpl();
	}
};

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
