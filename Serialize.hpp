#pragma once

#include <Utils.hpp>
#include <DataStorage.hpp>

#include <type_traits>
#include <cstring>

//#include <ObjectStorage.hpp>

template<typename T>
class ObjectStorage{

};
class ObjectAddress{

};

template<typename T, typename U>
concept CStorage = std::is_same_v<T, ObjectStorage<U>> || std::is_same_v<T, DataStorage>;
template<typename T>
concept CStorageAddress = std::is_same_v<T, StorageAddress> || std::is_same_v<T, ObjectAddress>;
//template<class T, class U>
//concept Derived = std::is_base_of<U, T>::value;

template<typename T>
concept CBuiltinSerializable = std::is_same_v<T, std::string> || std::is_trivially_copyable_v<T>;
template<typename T>
concept CSerializableImpl = requires(const T &t, const StorageBuffer &buffer,
                                     const StorageBufferRO &ro_buffer){
    { t.getSizeImpl() } -> std::convertible_to<std::size_t>;
    { t.serializeImpl(buffer) } -> std::same_as<Result>;
    { T::deserializeImpl(ro_buffer) } -> std::same_as<T>;
};
template<typename T>
concept CSerializable = CBuiltinSerializable<T> || CSerializableImpl<T>;

class Serialize{
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
        obj.serializeImpl(buffer);
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
        T obj = T::deserializeImpl(buffer);
        storage.commit(buffer);
        return obj;
    }
    template<CSerializableImpl T>
    static size_t getSize(const T &obj){
        size_t size = obj.getSizeImpl();
        return size;
    }
    //virtual ~Serialize(){}
#if 0
    Result serializeImpl(const StorageBuffer &buffer) const { throw std::bad_function_call("Not implemented"); }
    static T deserializeImpl(const StorageBufferRO &buffer) { throw std::bad_function_call("Not implemented"); }
    size_t getSizeImpl() const { throw std::bad_function_call("Not implemented"); }
#endif
};

/****************************************************/

template<typename T>
class BuiltinSerializeImpl{
public:
    BuiltinSerializeImpl(const T *obj) {}
};

template<typename T>
    requires std::is_trivially_copyable_v<T>
class BuiltinSerializeImpl<T>{
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
    static BuiltinSerializeImpl<T> deserializeImpl(const StorageBufferRO &buffer) {
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

template<>
class BuiltinSerializeImpl<std::string>{
    const std::string obj;
public:
    BuiltinSerializeImpl(const std::string &obj): obj(obj) {}
    Result serializeImpl(const StorageBuffer &buffer) const {
        if(buffer.size < getSizeImpl())
            throw std::out_of_range("Buffer size too small");
        char *s = static_cast<char *>(buffer.data);
        std::strncpy(s, obj.c_str(), buffer.size);
        return Result::Success;
    }
    static BuiltinSerializeImpl<std::string> deserializeImpl(const StorageBufferRO &buffer) {
        auto str = std::string{static_cast<const char *>(buffer.data), buffer.size - 1};
        // removeing trailing \0
        return BuiltinSerializeImpl{std::string{str.c_str()}};
    }
    size_t getSizeImpl() const {
        return obj.length() + 1;
    }
    const std::string &getObj() const { return obj; }
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
