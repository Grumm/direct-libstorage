#pragma once

#include <storage/SerializeImpl.hpp>

/****************************************************/

class DataStorage;

class Serialize{ //TODO not a class
public:
#if 0
    ObjectAddress serialize(ObjectStorage<T> &storage) const{
        StorageBuffer buffer = storage.addRawObjectStart();
        auto result = static_cast<T*>(this)->serializeImpl(buffer);
        return storage.addRawObjectFinish(buffer);
    }
#endif
    template<CSerializableImpl T>
    static void serialize(const T &obj, DataStorage &storage, StorageAddress &address){
        size_t size = getSize(obj);

        if(address.size < size){
            //invalidate old_address, create new
            //TODO maybe try to expand it first
            if(address.size != 0)
                storage.erase(address);
            address = storage.get_random_address(size);
        }
        StorageBuffer buffer = storage.writeb(address);
        szeimpl::s(obj, buffer);
        storage.commit(buffer);
    }
    template<CSerializableImpl T>
    static StorageAddress serialize(const T &obj, DataStorage &storage) {
        StorageAddress address;
        serialize(obj, storage, address);
        return address;
    }
#if 0
    static T deserialize(ObjectStorage<T> &storage, const ObjectAddress &addr){
        StorageBufferRO buffer = storage.getRawObject(addr);
        T obj = T::deserializeImpl(buffer);
        storage.putRawObject(buffer);
        return obj;
    }
#endif
    template<CSerializableImpl T>
    static T deserialize(DataStorage &storage, const StorageAddress &addr){
        StorageBufferRO buffer = storage.readb(addr);
        T obj = szeimpl::d<T>(buffer);
        storage.commit(buffer);
        return obj;
    }
    template<CSerializableImpl T>
    static size_t getSize(const T &obj){
        return szeimpl::size(obj);
    }
    //virtual ~Serialize(){}
#if 0
    Result serializeImpl(StorageBuffer<> &buffer) const { throw std::bad_function_call("Not implemented"); }
	template<typename T>
    static T deserializeImpl(const StorageBufferRO<> &buffer) { throw std::bad_function_call("Not implemented"); }
    size_t getSizeImpl() const { throw std::bad_function_call("Not implemented"); }
#endif
};

/****************************************************/

template<CStorageAddress A, CSerializable T, CStorage<T> S>
A serialize(S &storage, const T &obj){
    return Serialize::serialize(obj, storage);
}

template<CStorageAddress A, CSerializable T, CStorage<T> S>
void serialize(S &storage, const T &obj, A &address){
    Serialize::serialize(obj, storage, address);
}

template<CSerializable T, CStorageAddress A, CStorage<T> S>
T deserialize(S &storage, const A &address){
    return Serialize::deserialize<T>(storage, address);
}

template<CSerializable T, CStorageAddress A, CStorage<T> S>
std::unique_ptr<T> deserialize_ptr(S &storage, const A &address){
    return std::make_unique<T>(deserialize<T, A, S>(storage, address));
}
//for builtins

template<CStorageAddress A, CBuiltinSerializable T, CStorage<T> S>
A serialize(S &storage, const T &obj){
    return Serialize::serialize(BuiltinSerializeImpl<T>{obj}, storage);
}

template<CStorageAddress A, CBuiltinSerializable T, CStorage<T> S>
void serialize(S &storage, const T &obj, A &address){
    Serialize::serialize(BuiltinSerializeImpl<T>{obj}, storage, address);
}

template<CBuiltinSerializable T, CStorageAddress A, CStorage<T> S>
T deserialize(S &storage, const A &address){
    return Serialize::deserialize<BuiltinSerializeImpl<T>>(storage, address).getObj();
}

template<CBuiltinSerializable T, CStorageAddress A, CStorage<T> S>
std::unique_ptr<T> deserialize_ptr(S &storage, const A &address){
    return std::make_unique<T>(deserialize<T, A, S>(storage, address));
}
