#pragma once

#include <type_traits>

#include <storage/Utils.hpp>
#include <storage/StorageUtils.hpp>

//#include <ObjectStorage.hpp>

template<typename T>
class ObjectStorage{

};
class ObjectAddress{

};

class DataStorage;
class StorageAddress;

template<class T, class U>
concept Derived = std::is_base_of<U, T>::value;

template<typename T, typename U>
concept CStorage = std::is_same_v<T, ObjectStorage<U>> || std::is_same_v<T, DataStorage> || std::is_base_of_v<DataStorage, T>;
template<typename T, typename U = void>
concept CStorageImpl = std::is_same_v<T, ObjectStorage<U>> || (std::is_base_of_v<DataStorage, T> && !std::is_same_v<T, DataStorage>);
template<typename T>
concept CStorageAddress = std::is_same_v<T, StorageAddress> || std::is_same_v<T, ObjectAddress>;
template<typename T>
concept TupleLike = requires (T a) {
    std::tuple_size<T>::value;
    std::get<0>(a);
};

template <typename>
struct dsptr_T : std::false_type {};
template <typename T, typename ...Args>
struct dsptr_T<T(T::*)(const StorageBufferRO<> &, Args...)> : std::true_type {};
template <typename T>
concept CDeserializeImpl = dsptr_T<decltype(&T::deserializeImpl)>::value;

template<typename T>
concept CSerializableImpl = requires(const T &t, StorageBuffer<> &buffer,
                                     const StorageBufferRO<> &ro_buffer){
    { t.getSizeImpl() } -> std::convertible_to<std::size_t>;
    { t.serializeImpl(buffer) } -> std::same_as<Result>;
    CDeserializeImpl<T>;
    //{ T::deserializeImpl(ro_buffer, auto...) } -> std::same_as<T>;
};
template<typename T>
concept CBuiltinSerializable = (std::is_same_v<T, std::string>
                                || TupleLike<T>
                                || std::is_trivially_copyable_v<T>) && !CSerializableImpl<T>;
template<typename T>
concept CSerializable = CBuiltinSerializable<T> || CSerializableImpl<T>;

/****************************************************/
template<typename T>
class BuiltinSerializeImpl;

template<typename T>
using base_type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

namespace szeimpl{

template<CSerializableImpl T>
size_t size(const T &t){
    return t.getSizeImpl();
}

template<CBuiltinSerializable T>
size_t size(const T &t){
    return BuiltinSerializeImpl<T>{t}.getSizeImpl();
}

template<CSerializableImpl T, typename U>
constexpr Result s(const T &t, StorageBuffer<U> &buffer){
    return t.template serializeImpl(buffer.template cast<void>());
}

template<CBuiltinSerializable T, typename U>
constexpr Result s(const T &t, StorageBuffer<U> &buffer){
    return BuiltinSerializeImpl<std::remove_cvref_t<T>>{t}.template serializeImpl(buffer.template cast<void>());
}

template<CSerializableImpl T, typename U, typename ...Args>
T d(const StorageBufferRO<U> &buffer, Args&&... args) {
    return std::remove_cvref_t<T>::deserializeImpl(buffer.template cast<void>(), std::forward<Args>(args)...);
}

template<CBuiltinSerializable T, typename U, typename ...Args>
T d(const StorageBufferRO<U> &buffer, Args&&... args) {
    return BuiltinSerializeImpl<std::remove_cvref_t<T>>::
        deserializeImpl(buffer.template cast<void>(), std::forward<Args>(args)...).getObj();
}

}

/****************************************************/

template<typename T>
class BuiltinSerializeImpl{
public:
    BuiltinSerializeImpl(const T *obj) {}
};

/****************************************************/

template <typename T>
struct SerializableMap{
    size_t size;
    T data[0];
};
