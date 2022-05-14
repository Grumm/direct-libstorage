#pragma once

#include <type_traits>
#include <memory>
#include <cstring>
#include <array>

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
template<typename T>
concept CStorageAddress = std::is_same_v<T, StorageAddress> || std::is_same_v<T, ObjectAddress>;
template<typename T>
concept TupleLike = requires (T a) {
    std::tuple_size<T>::value;
    std::get<0>(a);
};
template<typename T>
concept CSerializableImpl = requires(const T &t, StorageBuffer<> &buffer,
                                     const StorageBufferRO<> &ro_buffer){
    { t.getSizeImpl() } -> std::convertible_to<std::size_t>;
    { t.serializeImpl(buffer) } -> std::same_as<Result>;
    { T::deserializeImpl(ro_buffer) } -> std::same_as<T>;
};
template<typename T, typename ...U>
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

template<CSerializableImpl T, typename U>
T d(const StorageBufferRO<U> &buffer) {
    return std::remove_cvref_t<T>::deserializeImpl(buffer.template cast<void>());
}

template<CBuiltinSerializable T, typename U>
T d(const StorageBufferRO<U> &buffer) {
    return BuiltinSerializeImpl<std::remove_cvref_t<T>>::
        deserializeImpl(buffer.template cast<void>()).getObj();
}

}

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
    using T_NoConst = std::remove_const_t<T>;
public:
    BuiltinSerializeImpl(const T &obj): obj(obj) {}
    Result serializeImpl(StorageBuffer<> &buffer) const {
        ASSERT_ON_MSG(buffer.size() < getSizeImpl(), "Buffer size too small");
        T_NoConst *t = buffer.template get<T_NoConst>();
        *t = obj;
        return Result::Success;
    }
    static BuiltinSerializeImpl<T> deserializeImpl(const StorageBufferRO<> &buffer) {
        const T *t = buffer.template get<T>();
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
    Result serializeImpl(StorageBuffer<> &buffer) const {
        ASSERT_ON_MSG(buffer.size() < getSizeImpl(), "Buffer size too small");
        char *s = buffer.get<char>();
        std::strncpy(s, obj.c_str(), buffer.size());
        return Result::Success;
    }
    static BuiltinSerializeImpl<std::string> deserializeImpl(const StorageBufferRO<> &buffer) {
        auto str = std::string{buffer.get<const char>(), buffer.size() - 1};
        // removing trailing \0
        return BuiltinSerializeImpl{std::string{str.c_str()}};
    }
    size_t getSizeImpl() const {
        return obj.length() + 1;
    }
    const std::string &getObj() const { return obj; }
};

template <typename T>
class BSIWrapper2{
    const T *obj;
    std::array<uint8_t, sizeof(T)> data;
};

template<CSerializable F, CSerializable S>
class BuiltinSerializeImpl<std::pair<F, S>>{
    using T = std::pair<F, S>;
    const T *obj;
    std::array<uint8_t, sizeof(T)> data;
public:
    BuiltinSerializeImpl(const T &obj): obj(&obj) {}
    BuiltinSerializeImpl(const F &f, const S &s) {
        obj = new (data.data()) T(f, s);
    }
    Result serializeImpl(StorageBuffer<> &buffer) const {
        ASSERT_ON_MSG(buffer.size() < getSizeImpl(), "Buffer size too small");
        Result res = Result::Success;

        size_t offset = 0;
        StorageBuffer buf = buffer.offset_advance(offset, szeimpl::size(obj->first));
        res = szeimpl::s(obj->first, buf);
        buf = buffer.offset_advance(offset, szeimpl::size(obj->second));
        res = szeimpl::s(obj->second, buf);

        return res;
    }
    static BuiltinSerializeImpl<T> deserializeImpl(const StorageBufferRO<> &buffer) {
        size_t offset = 0;
        StorageBufferRO buf = buffer;
        F f = szeimpl::d<F>(buf);
        buffer.offset_advance(offset, szeimpl::size(f));
        buf = buffer.offset(offset, buffer.size() - offset);
        S s = szeimpl::d<S>(buf);

        return BuiltinSerializeImpl{f, s};
    }
    size_t getSizeImpl() const {
        size_t sum = 0;
        std::apply([&sum](auto&&... arg){
            ((sum += szeimpl::size(arg)),
            ...);
        }, *obj);
        return sum;
    }
    const T getObj() const { return *obj; }
};

template <std::size_t ... Is>
constexpr auto indexSequenceReverse (std::index_sequence<Is...> const &)
   -> decltype( std::index_sequence<sizeof...(Is)-1U-Is...>{} );
template <std::size_t N>
using makeIndexSequenceReverse = decltype(indexSequenceReverse(std::make_index_sequence<N>{}));

template<typename T, size_t... I>
auto
reverse_impl(T&& t, std::index_sequence<I...>)
{
  return std::make_tuple(std::get<sizeof...(I) - 1 - I>(std::forward<T>(t))...);
}

template<typename T>
auto reverse_tuple(T&& t)
{
  return reverse_impl(std::forward<T>(t),
                      std::make_index_sequence<std::tuple_size<T>::value>());
}

template<typename Tuple, typename F, std::size_t ...I>
auto apply_impl(F& f, std::index_sequence<I...>) {
    return std::make_tuple(f.template operator()<std::tuple_element_t<I, Tuple>>()...);
}
template<typename Tuple, typename F>
auto apply_make_tuple(F& f) {
    using Indices = makeIndexSequenceReverse<std::tuple_size<std::remove_cvref_t<Tuple>>::value>;
    return reverse_tuple(apply_impl<Tuple, F>(f, Indices()));
}

template<CSerializable ...TArgs>
class BuiltinSerializeImpl<std::tuple<TArgs...>>{
    const std::tuple<TArgs...> obj;
public:
    BuiltinSerializeImpl(const std::tuple<TArgs...> &obj): obj(obj) {}
    Result serializeImpl(StorageBuffer<> &buffer) const {
        ASSERT_ON_MSG(buffer.size() < getSizeImpl(), "Buffer size too small");
        Result res = Result::Success;
        size_t offset = 0;
        StorageBuffer buf;
        std::apply([&buf, buffer, &offset, &res](auto&&... arg){
            ((
                buf = buffer.offset_advance(offset, szeimpl::size(arg)),
                res = szeimpl::s<std::remove_cvref_t<decltype(arg)>>(arg, buf) /*TODO res && */
            ),
                ...);
        }, obj);
        return res;
    }

    static BuiltinSerializeImpl<std::tuple<TArgs...>> deserializeImpl(const StorageBufferRO<> &buffer) {
        size_t offset = 0;
        StorageBufferRO buf = buffer;
        auto deserialize_argument = [&buf, buffer, &offset]<typename T>() -> T{
            buf = buffer.offset(offset, buffer.size() - offset);
            T t = szeimpl::d<T>(buf);
            offset += szeimpl::size(t);
            return t;
        };
        auto tuple = apply_make_tuple<std::tuple<TArgs...>>(deserialize_argument);
        return BuiltinSerializeImpl(tuple);
    }
#if 0
    template<CSerializable ...TArgs>
    static BuiltinSerializeImpl<std::tuple<TArgs...>> deserializeImpl(const StorageBufferRO<> &buffer) {
        size_t offset = 0;
        StorageBufferRO buf = buffer;
        std::tuple<TArgs...> tuple{};
        std::apply([&buf, buffer, &offset](auto&... arg){
            ((
                buf = buffer.offset(offset, buffer.size() - offset),
                *(const_cast<std::remove_const_t<std::remove_reference_t<decltype(arg)>> *>(&arg)) =
                    szeimpl::d<std::remove_reference_t<decltype(arg)>>(buf),
                offset += szeimpl::size(arg)
            ),
                ...);
        }, tuple);
        return BuiltinSerializeImpl{tuple};
    }
#endif
    size_t getSizeImpl() const {
        size_t sum = 0;
        std::apply([&sum](auto&&... arg){
            ((sum += szeimpl::size(arg)),
            ...);
        }, obj);
        return sum;
    }
    const std::tuple<TArgs...> &getObj() const { return obj; }
};

template<CSerializable T>
class BuiltinSerializeImpl<std::unique_ptr<T>>{
public:
    BuiltinSerializeImpl(const std::unique_ptr<T> &obj) {}
    Result serializeImpl(StorageBuffer<> &buffer) const {
        return Result::Success;
    }
    static BuiltinSerializeImpl<std::unique_ptr<T>> deserializeImpl(const StorageBufferRO<> &buffer) {
        return BuiltinSerializeImpl{std::unique_ptr<T>{}};
    }
    size_t getSizeImpl() const {
        return 0;
    }
    const std::unique_ptr<T> &getObj() const { return std::unique_ptr<T>{}; }
};

#if 0
template<typename T>
concept ElementIterable = std::ranges::range<std::ranges::range_value_t<T>>;
template<CSerializable T, ElementIterable<T> U>
class BuiltinSerializeImpl<std::map<Key, Val>>{
    const std::string obj;
public:
    BuiltinSerializeImpl(const std::string &obj): obj(obj) {}
    Result serializeImpl(StorageBuffer<> &buffer) const {
        ASSERT_ON_MSG(buffer.size() < getSizeImpl(), "Buffer size too small");
        char *s = buffer.get<char>();
        std::strncpy(s, obj.c_str(), buffer.size());
        return Result::Success;
    }
    static BuiltinSerializeImpl<std::string> deserializeImpl(const StorageBufferRO<> &buffer) {
        auto str = std::string{buffer.get<const char>(), buffer.size() - 1};
        // removing trailing \0
        return BuiltinSerializeImpl{std::string{str.c_str()}};
    }
    size_t getSizeImpl() const {
        return obj.length() + 1;
    }
    const std::string &getObj() const { return obj; }
};
#endif

/****************************************************/

template <typename T>
struct SerializableMap{
    size_t size;
    T data[0];
};
