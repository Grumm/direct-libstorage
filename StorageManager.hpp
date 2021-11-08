#pragma once

class StorageManager{
public:
	using StorageIdType = uint32_t;

	template<typename T, typename K>
	ObjectStorage<T,K> &getObjectStorageInstance(StorageIdType id);

	DataStorage &getDataStorageInstance(StorageIdType id);
};