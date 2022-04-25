#pragma once

#include <Utils.hpp>
#include <DataStorage.hpp>
#include <ObjectStorage.hpp>



template<typename T, typename U>
concept CStorage = is_same_v<U, ObjectStorage<T>> || is_same_v<U, DataStorage>;
template<typename T>
concept CStorageAddress = is_same_v<T, StorageAddress> || is_same_v<T, ObjectAddress>;
//template<class T, class U>
//concept Derived = std::is_base_of<U, T>::value;

template<typename T>
concept CBuiltinSerializable = is_same_v<T, std::string> || is_trivially_copyable_v<T>;
template<typename T>
concept CSerializableImpl = requires(const T &t, const StorageBuffer &buffer,
                                     const StorageBufferRO &ro_buffer){
    { t.getSizeImpl(ro_buffer) } -> std::convertible_to<std::size_t>;
    { t.serializeImpl(buffer) } -> Result;
    { T::deserializeImpl(ro_buffer) } -> T;
};
template<CSerializableImpl T>
class ISerialize;
template<typename T>
concept CSerializable = CBuiltinSerializable<T>
                        || (CSerializableImpl<T> && DerivedFrom<T, ISerialize<T>>);


template<CSerializableImpl T>
class ISerialize{
public:
#if 0
    ObjectAddress serialize(ObjectStorage<T> &storage) const{
        StorageBuffer buffer = storage.addRawObjectStart();
        auto result = static_cast<T*>(this)->serializeImpl(buffer);
        return storage.addRawObjectFinish(buffer);
    }
#endif
    void serialize(DataStorage &storage, const StorageAddress &address) const{
        T *obj = dynamic_cast<T*>(this);
        size_t size = obj->getSizeImpl();

        if(old_address.size < size){
            //invalidate old_address, create new
            //TODO maybe try to expand it first
            if(address.size != 0)
                storage.erase(address);
            address = storage.get_random_address(size);
        }
        StorageBuffer buffer = storage.writeb(address);
        obj->serializeImpl(buffer);
        storage.commit(buffer);
        return address;
    }
    StorageAddress serialize(DataStorage &storage) const{
        StorageAddress address;
        serialize(storage, address);
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
    static U deserialize(DataStorage &storage, const StorageAddress &addr){
        StorageBufferRO buffer = storage.readb(addr);
        U obj = T::deserializeImpl(buffer);
        storage.commit(buffer);
        return obj;
    }
    size_t getSize() const{
        T *obj = dynamic_cast<T*>(this);
        size_t size = obj->getSizeImpl();
        return size;
    }
#if 0
    Result serializeImpl(const StorageBuffer &buffer) const { throw std::bad_function_call("Not implemented"); }
    static T deserializeImpl(const StorageBufferRO &buffer) { throw std::bad_function_call("Not implemented"); }
    size_t getSizeImpl() const { throw std::bad_function_call("Not implemented"); }
#endif
};

template<typename T>
class BuiltinSerializeImpl{
public:
    BuiltinSerializeImpl(const T *obj) {}
};

template<typename T>
    requires std::is_trivially_copyable_v<T>
class BuiltinSerializeImpl{
    const T obj;
public:
    BuiltinSerializeImpl(const T &obj): obj(obj) {}
    Result serializeImpl(const StorageBuffer &buffer) const {
        if(buffer.size < sizeof(T))
            throw std::out_of_range("Buffer size too small");
        T *t = static_cast<T*>(buffer.data);
        *t = obj;
        return Result::Success;
    }
    static T deserializeImpl(const StorageBufferRO &buffer) {
        if(buffer.size < sizeof(T))
            throw std::out_of_range("Buffer size too small");
        const T *t = static_cast<const T *>(buffer.data);
        return BuiltinSerializeImpl{*t};
    }
    size_t getSizeImpl() const {
        return sizeof(T);
    }
    const T &getObj() const { return obj; }
};

class BuiltinSerializeImpl<std::string>{
    const std::string obj;
public:
    BuiltinSerializeImpl(const std::string &obj): obj(obj) {}
    Result serializeImpl(const StorageBuffer &buffer) const {
        if(buffer.size < getSizeImpl())
            throw std::out_of_range("Buffer size too small");
        char *s = static_cast<char *>(buffer.data);
        strncpy(s, obj->c_str(), buffer.size());
        return Result::Success;
    }
    static std::string deserializeImpl(const StorageBufferRO &buffer) {
        return std::string{static_cast<const char *>(buffer.data), buffer.size};
    }
    size_t getSizeImpl() const {
        return obj->length() + 1;
    }
    const T &getObj() const { return obj; }
};

template<CSerializable T, CStorageAddress A, CStorage S>
A serialize(S &storage, const T &obj){
    auto &obj_ser = static_cast<ISerialize<T> &>(obj);
    return obj_ser.serialize(storage);
}

template<CSerializable T, CStorageAddress A, CStorage S>
void serialize(S &storage, const T &obj, A &address){
    auto &obj_ser = static_cast<ISerialize<T> &>(obj);
    obj_ser.serialize(storage, address);
}

template<CSerializable T, CStorageAddress A, CStorage S>
T deserialize(S &storage, const A &address){
    return ISerialize<T>::deserialize(storage, address);
}

template<CSerializable T, CStorageAddress A, CStorage S>
std::unique_ptr<T> deserialize_ptr(S &storage, const A &address){
    return std::make_unique<T>(new T(deserialize<T, A, S>(storage, address)));
}
//for builtins

template<CBuiltinSerializable T, CStorageAddress A, CStorage S>
A serialize(S &storage, const T &obj){
    auto &obj_ser = static_cast<ISerialize<BuiltinSerializeImpl<T>> &>(obj);
    return obj_ser.serialize(storage);
}

template<CBuiltinSerializable T, CStorageAddress A, CStorage S>
void serialize(S &storage, const T &obj, A &address){
    auto &obj_ser = static_cast<ISerialize<BuiltinSerializeImpl<T>> &>(obj);
    obj_ser.serialize(storage, address);
}

template<CBuiltinSerializable T, CStorageAddress A, CStorage S>
T deserialize(S &storage, const A &address){
    return ISerialize<BuiltinSerializeImpl<T>>::deserialize(storage, address).getObj();
}

template<CBuiltinSerializable T, CStorageAddress A, CStorage S>
std::unique_ptr<T> deserialize_ptr(S &storage, const A &address){
    return std::make_unique<T>(new T(deserialize<T, A, S>(storage, address)));
}

/****************************************************/

class SerializableImplExample:
    public ISerialize<SerializableImplExample>{
public:
    Result serializeImpl(const StorageBuffer &buffer) const { throw std::bad_function_call("Not implemented"); }
    static SerializableImplExample deserializeImpl(const StorageBufferRO &buffer) {
        throw std::bad_function_call("Not implemented"); 
    }
    size_t getSizeImpl() const { throw std::bad_function_call("Not implemented"); }
};
