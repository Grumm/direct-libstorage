#pragma once

#include <storage/SerializeImpl.hpp>
#include <storage/SerializeSpecialization.hpp>

/****************************************************/

class DataStorage;

namespace sze{
#if 0
    ObjectAddress serialize(ObjectStorage<T> &storage) const{
        StorageBuffer buffer = storage.addRawObjectStart();
        auto result = static_cast<T*>(this)->serializeImpl(buffer);
        return storage.addRawObjectFinish(buffer);
    }
    static T deserialize(ObjectStorage<T> &storage, const ObjectAddress &addr){
        StorageBufferRO buffer = storage.getRawObject(addr);
        T obj = T::deserializeImpl(buffer);
        storage.putRawObject(buffer);
        return obj;
    }
#endif
    template<CSerializable T>
    size_t getSize(const T &obj){
        return szeimpl::size(obj);
    }
    template<CSerializable T>
    void serialize(const T &obj, DataStorage &storage, StorageAddress &address){
        size_t size = getSize<T>(obj);

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
    template<CSerializable T>
    StorageAddress serialize(const T &obj, DataStorage &storage) {
        StorageAddress address;
        serialize(obj, storage, address);
        return address;
    }

    template<typename T, typename ...Args>
    requires CDeserializableImpl<T, Args...>
    T deserialize(DataStorage &storage, const StorageAddress &addr, Args&&... args){
        StorageBufferRO buffer = storage.readb(addr);
        ScopeDestructor sd{[&storage, &buffer](){ storage.commit(buffer); }};
        return szeimpl::d<T, void, Args...>(buffer, std::forward<Args>(args)...);
    }
#if 0
    //interface
    Result serializeImpl(StorageBuffer<> &buffer) const { throw std::bad_function_call("Not implemented"); }
	template<typename T>
    static T deserializeImpl(const StorageBufferRO<> &buffer) { throw std::bad_function_call("Not implemented"); }
    size_t getSizeImpl() const { throw std::bad_function_call("Not implemented"); }
#endif
};

/****************************************************/

template<CStorageAddress A, CSerializableImpl T, CStorage<T> S>
A serialize(S &storage, const T &obj){
    return sze::serialize(obj, storage);
}

template<CStorageAddress A, CSerializableImpl T, CStorage<T> S>
void serialize(S &storage, const T &obj, A &address){
    sze::serialize(obj, storage, address);
}

template<typename T, CStorageAddress A, CStorage<T> S, typename ...Args>
requires CDeserializableImpl<T, Args...>
T deserialize(S &storage, const A &address, Args&& ...args){
    return sze::deserialize<T, Args...>(storage, address, std::forward<Args>(args)...);
}

template<typename T, CStorageAddress A, CStorage<T> S, typename ...Args>
requires CDeserializableImpl<T, Args...>
std::unique_ptr<T> deserialize_ptr(S &storage, const A &address, Args&& ...args){
    T *t = new T{deserialize<T, A, S, Args...>(storage, address, std::forward<Args>(args)...)};
    return std::unique_ptr<T>(t);
}
//for builtins

template<CStorageAddress A, CBuiltinSerializable T, CStorage<T> S>
A serialize(S &storage, const T &obj){
    return sze::serialize(BuiltinSerializeImpl<T>{obj}, storage);
}

template<CStorageAddress A, CBuiltinSerializable T, CStorage<T> S>
void serialize(S &storage, const T &obj, A &address){
    sze::serialize(BuiltinSerializeImpl<T>{obj}, storage, address);
}

template<typename T, CStorageAddress A, CStorage<T> S, typename ...Args>
requires CBuiltinDeserializable<T, Args...>
T deserialize(S &storage, const A &address, Args&& ...args){
    return sze::deserialize<BuiltinDeserializeImpl<T>, Args...>(storage, address, std::forward<Args>(args)...).getObj();
}

template<typename T, CStorageAddress A, CStorage<T> S, typename ...Args>
requires CBuiltinDeserializable<T, Args...>
std::unique_ptr<T> deserialize_ptr(S &storage, const A &address, Args&& ...args){
    return std::make_unique<T>(deserialize<T, A, S, Args...>(storage, address, std::forward<Args>(args)...));
}
