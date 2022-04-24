#pragma once

#include <Utils.hpp>
#include <DataStorage.hpp>
#include <ObjectStorage.hpp>

template<typename T>
class ISerializable;


template<typename T, typename U>
concept CStorage = is_same_v<U, ObjectStorage<T>> || is_same_v<U, DataStorage>;
template<typename T>
concept CStorageAddress = is_same_v<T, StorageAddress> || is_same_v<T, ObjectAddress>;
template<class T, class U>
concept Derived = std::is_base_of<U, T>::value;

template<typename T>
concept CBuiltinSerializable = is_same_v<T, std::string>
                                || is_arithmetic_v<T> || is_enum_v<T>;
template<typename T>
concept CSerializableImpl = requires(const T &t, const StorageBuffer &buffer,
                                     const StorageBufferRO &ro_buffer){
    { t.getSizeImpl(ro_buffer) } -> std::convertible_to<std::size_t>;
    { t.serializeImpl(buffer) } -> Result;
    { T::deserializeImpl(ro_buffer) } -> T;
};
template<typename T>
concept CSerializable = CBuiltinSerializable<T> || (CSerializableImpl<T> && Derived<T, ISerializable<T>>);


template<CSerializableImpl T>
class ISerializable{
    // Original address?
public:
#if 0
    ObjectAddress serialize(ObjectStorage<T> &storage) const{
        StorageBuffer buffer = storage.addRawObjectStart();
        auto result = static_cast<T*>(this)->serializeImpl(buffer);
        return storage.addRawObjectFinish(buffer);
    }
#endif
    StorageAddress serialize(DataStorage &storage) const{
        size_t size = getSize();
        StorageAddress address = storage.get_random_address(size);
        StorageBuffer buffer = storage.writeb(address);
        T *obj = dynamic_cast<T*>(this);
        obj->serializeImpl(buffer);
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
    static T deserialize(DataStorage &storage, const StorageAddress &addr){
        StorageBufferRO buffer = storage.readb(addr);
        T &&obj = T::deserializeImpl(buffer);
        storage.putRawObject(raw_buffer);
        return obj;
    }
    size_t getSize() const{
        T *obj = static_cast<T*>(this);
        size_t size = obj->getSizeImpl();
        return size;
    }
#if 0
    Result serializeImpl(const StorageBuffer &buffer) const { throw std::bad_function_call("Not implemented"); }
    static T deserializeImpl(const StorageBufferRO &buffer) { throw std::bad_function_call("Not implemented"); }
    size_t getSizeImpl() const { throw std::bad_function_call("Not implemented"); }
#endif
};

template<CSerializable T, CStorageAddress A, CStorage S>
A serialize(S &storage, const T &obj){
    auto &obj_ser = static_cast<ISerializable<T> &>(obj);
    return obj_ser.serialize(storage);
}

template<CSerializable T, CStorageAddress A, CStorage S>
T deserialize(S &storage, const A &address){
    return ISerializable<T>::deserialize(storage, address);
}

template<CSerializable T, CStorageAddress A, CStorage S>
std::unique_ptr<T> deserialize_ptr(S &storage, const A &address){
    return std::make_unique<T>(new T(deserialize(storage, address)));
}

/****************************************************/

class SerializableImplExample:
    public ISerializable<SerializableImplExample>{
public:
    Result serializeImpl(const StorageBuffer &buffer) const { throw std::bad_function_call("Not implemented"); }
    static SerializableImplExample deserializeImpl(const StorageBufferRO &buffer) {
        throw std::bad_function_call("Not implemented"); 
    }
    size_t getSizeImpl() const { throw std::bad_function_call("Not implemented"); }
};
