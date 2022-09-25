#pragma once

#include <memory>
#include <cstring>
#include <array>

#include <storage/SerializeImpl.hpp>

template <typename T>
class BSIObjectWrapper{
    const T *obj;
    std::array<uint8_t, sizeof(T)> data;
public:
    BSIObjectWrapper(const T &obj): obj(&obj) {}
    BSIObjectWrapper(const T *obj): obj(obj) {}
    template<typename ...Args>
    BSIObjectWrapper(Args &&...args): obj(new (data.data()) T(std::forward<Args>(args)...)) {}

    const T *getObjPtr() const{ return obj; }
    const T getObj() const{ return *obj; }
};

template<typename T>
    requires std::is_trivially_copyable_v<T>
class BuiltinSerializeImpl<T>: public BSIObjectWrapper<T>{
    using T_NoConst = std::remove_const_t<T>;
public:
    BuiltinSerializeImpl(const T &obj): BSIObjectWrapper<T>(obj) {}
    BuiltinSerializeImpl(const T *obj): BSIObjectWrapper<T>(obj) {}

    Result serializeImpl(StorageBuffer<> &buffer) const {
        ASSERT_ON_MSG(buffer.size() < getSizeImpl(), "Buffer size too small");
        T_NoConst *t = buffer.template get<T_NoConst>();
        *t = *this->template getObjPtr();
        return Result::Success;
    }
    static BuiltinSerializeImpl<T> deserializeImpl(const StorageBufferRO<> &buffer) {
        const T *t = buffer.template get<T>();
        return BuiltinSerializeImpl<T>{t};
    }
    size_t getSizeImpl() const {
        return sizeof(T);
    }
};

template<>
class BuiltinSerializeImpl<std::string>: public BSIObjectWrapper<std::pair<const char *, size_t>>{
    using T = std::string;
    using U = std::pair<const char *, size_t>;
    const T *obj{nullptr};
public:
    BuiltinSerializeImpl(const T &obj): BSIObjectWrapper<U>(nullptr, 0), obj(&obj) {}
    BuiltinSerializeImpl(const char *c_str, size_t len): BSIObjectWrapper<U>(c_str, len) {}

    Result serializeImpl(StorageBuffer<> &buffer) const {
        ASSERT_ON_MSG(buffer.size() < getSizeImpl(), "Buffer size too small");
        char *s = buffer.get<char>();
        std::strncpy(s, obj->c_str(), buffer.size());
        return Result::Success;
    }
    static BuiltinSerializeImpl<T> deserializeImpl(const StorageBufferRO<> &buffer) {
        // removing trailing \0
        return BuiltinSerializeImpl(buffer.get<const char>(), buffer.size() - 1);
    }
    size_t getSizeImpl() const {
        if(obj == nullptr)
            return this->getObjPtr()->second + 1;
        else 
            return obj->length() + 1;
    }
    const std::string getObj() const {
        if(obj == nullptr){
            auto [c_str, len] = *this->getObjPtr();
            return std::string{c_str, ::strnlen(c_str, len)};
        } else {
            return *obj;
        }
    }
};

template<CSerializable F, CSerializable S>
class BuiltinSerializeImpl<std::pair<F, S>>: public BSIObjectWrapper<std::pair<F, S>>{
    using T = std::pair<F, S>;
public:
    BuiltinSerializeImpl(const T &obj): BSIObjectWrapper<T>(obj) {}
    BuiltinSerializeImpl(const F &f, const S &s): BSIObjectWrapper<T>(f, s) {}

    Result serializeImpl(StorageBuffer<> &buffer) const {
        ASSERT_ON_MSG(buffer.size() < getSizeImpl(), "Buffer size too small");
        Result res = Result::Success;

        size_t offset = 0;
        auto *obj = this->template getObjPtr();
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
        }, *this->template getObjPtr());
        return sum;
    }
};

/******************/

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
/******************/

template<typename U, typename T, size_t... I>
static U instantiate_from_tuple_impl(T &&t, std::index_sequence<I...>){
    return U{std::get<I>(std::forward<T>(t))...};
}

template<typename U, typename T, size_t... I>
static U instantiate_from_tuple(T &&t){
    return instantiate_from_tuple_impl<U>(std::forward<T>(t), std::make_index_sequence<std::tuple_size<T>::value>());
}

template<CSerializable ...TArgs>
class BuiltinSerializeImpl<std::tuple<TArgs...>>: public BSIObjectWrapper<std::tuple<TArgs...>>{
    using T = std::tuple<TArgs...>;
public:
    BuiltinSerializeImpl(const T &obj): BSIObjectWrapper<T>(obj) {}
    BuiltinSerializeImpl(TArgs && ...args): BSIObjectWrapper<T>(std::forward<TArgs>(args)...) {}

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
        }, *this->template getObjPtr());
        return res;
    }

    template<size_t... I>
    static BuiltinSerializeImpl<T> instantiateFromTuple(T &&t, std::index_sequence<I...>){
        return BuiltinSerializeImpl<T>{std::get<I>(std::forward<T>(t))...};
    }

    static BuiltinSerializeImpl<T> deserializeImpl(const StorageBufferRO<> &buffer) {
        size_t offset = 0;
        StorageBufferRO buf = buffer;
        auto deserialize_argument = [&buf, buffer, &offset]<typename T>() -> T{
            buf = buffer.offset(offset, buffer.size() - offset);
            T t = szeimpl::d<T>(buf);
            offset += szeimpl::size(t);
            return t;
        };
        auto tuple = apply_make_tuple<T>(deserialize_argument);
        return instantiate_from_tuple<BuiltinSerializeImpl<T>>(std::move(tuple));
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
        }, *this->template getObjPtr());
        return sum;
    }
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
