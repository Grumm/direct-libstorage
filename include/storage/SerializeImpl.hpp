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

#if 0
template <typename>
struct dsptr_T : std::false_type {};
template <typename T, typename ...Args>
struct dsptr_T<T(*)(const StorageBufferRO<> &, Args...)> : std::true_type {};
template <typename T>
struct dsptr_T<T(*)(const StorageBufferRO<> &)> : std::true_type {};
template <typename T>
concept CDeserializableImpl = dsptr_T<decltype(&T::deserializeImpl)>::value;

#else
template<typename T, typename ...Args>
concept CDeserializableImpl1 = requires(const T &t, const StorageBufferRO<> &buffer, Args... args){
    { T::deserializeImpl(buffer, std::forward<Args>(args)...) } -> std::same_as<T>;
};
template<typename T, typename ...Args>
concept CDeserializableImpl2 = requires(const T &t, const StorageBufferRO<> &buffer, Args... args){
    { T::deserializeImpl(buffer) } -> std::same_as<T>;
};

template<typename T, typename ...Args>
concept CDeserializableImpl = CDeserializableImpl1<T, Args...> || CDeserializableImpl2<T>;
#endif

template<typename T>
concept CSerializableFixedSize = requires{
    { std::bool_constant<(T{}.getSizeImpl(), true)>() } -> std::same_as<std::true_type>;
};

template<typename T>
concept CSerializableImpl = requires(const T &t, StorageBuffer<> &buffer,
                                     const StorageBufferRO<> &ro_buffer){
    { t.getSizeImpl() } -> std::convertible_to<std::size_t>;
    { t.serializeImpl(buffer) } -> std::same_as<Result>;
    //{ T::deserializeImpl(ro_buffer, auto...) } -> std::same_as<T>;
};

template<typename T>
concept CBuiltinSerializable = (std::is_same_v<T, std::string>
                                || TupleLike<T>
                                || std::is_trivially_copyable_v<T>) && !CSerializableImpl<T>;

template<typename T, typename ...Args>
concept CBuiltinDeserializable = (std::is_same_v<T, std::string>
                                || TupleLike<T>
                                || std::is_trivially_copyable_v<T>) && !CDeserializableImpl<T, Args...>;
template<typename T, typename ...Args>
concept CSerializable = (CBuiltinSerializable<T> || CSerializableImpl<T>)
                                && (CBuiltinDeserializable<T> || CDeserializableImpl<T, Args...>);

/****************************************************/
template<typename T>
class BuiltinSerializeImpl;

template<typename T>
class BuiltinDeserializeImpl;

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
    LOG("Serialize: %s", typeid(T).name());
    return t.template serializeImpl(buffer.template cast<void>());
}

template<CBuiltinSerializable T, typename U>
constexpr Result s(const T &t, StorageBuffer<U> &buffer){
    LOG("BSerialize: %s", typeid(T).name());
    return BuiltinSerializeImpl<std::remove_cvref_t<T>>{t}.template serializeImpl(buffer.template cast<void>());
}

template<typename T, typename U, typename ...Args>
requires CDeserializableImpl<T, Args...>
T d(const StorageBufferRO<U> &buffer, Args&&... args) {
    LOG("Deserialize: %s", typeid(T).name());
    return std::remove_cvref_t<T>::deserializeImpl(buffer.template cast<void>(), std::forward<Args>(args)...);
}

template<typename T, typename U, typename ...Args>
requires CBuiltinDeserializable<T, Args...>
T d(const StorageBufferRO<U> &buffer, Args&&... args) {
    LOG("BDeserialize: %s", typeid(T).name());
    return BuiltinDeserializeImpl<std::remove_cvref_t<T>>::
        deserializeImpl(buffer.template cast<void>(), std::forward<Args>(args)...).getObj();
}

}

/****************************************************/

template<typename T>
class BuiltinSerializeImpl{
public:
};

template<typename T>
class BuiltinDeserializeImpl{
public:
};

/****************************************************/

template <typename T>
struct SerializableMap{
    size_t size;
    T data[0];
};
