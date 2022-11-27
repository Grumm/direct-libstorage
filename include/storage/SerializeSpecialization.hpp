#pragma once

#include <memory>
#include <cstring>
#include <functional>
#include <vector>
#include <array>
#include <bit>
#include <ranges>
#include <algorithm>

#include <storage/SerializeImpl.hpp>

template <typename T>
class BSIObjectWrapperSerialize{
    const T &obj;
public:
    BSIObjectWrapperSerialize(const T &obj): obj(obj) {}

    const T getObj() const{ return obj; }
    const T &getObjRef() const{ return obj; }
};

template <typename T>
class BSIObjectWrapperDeserialize{
    //const T *obj{nullptr};
protected:
    const StorageBufferRO<> buffer;
public:
    BSIObjectWrapperDeserialize(const StorageBufferRO<> &buffer): buffer(buffer) {}

    static BuiltinDeserializeImpl<T> deserializeImpl(const StorageBufferRO<> &buffer) {
        return BuiltinDeserializeImpl<T>{buffer};
    }
    T getObj() = delete;
};

/************************/
template<typename T>
    requires std::is_trivially_copyable_v<T>
class BuiltinSerializeImpl<T>: public BSIObjectWrapperSerialize<T>{
    using T_NoConst = std::remove_const_t<T>;
public:
    BuiltinSerializeImpl(const T &obj): BSIObjectWrapperSerialize<T>(obj) {}

    Result serializeImpl(StorageBuffer<> &buffer) const {
        ASSERT_ON_MSG(buffer.size() < getSizeImpl(), "Buffer size too small");
        T_NoConst *t = buffer.template get<T_NoConst>();
        *t = this->template getObj();
        return Result::Success;
    }
    constexpr size_t getSizeImpl() const {
        return sizeof(T);
    }
};

template<typename T>
    requires std::is_trivially_copyable_v<T>
class BuiltinDeserializeImpl<T>: public BSIObjectWrapperDeserialize<T>{
public:
    T getObj() const {
        return *this->buffer.template get<T>();
    }
};

/************************/

template<>
class BuiltinSerializeImpl<std::string>: public BSIObjectWrapperSerialize<std::string>{
    using T = std::string;
public:
    BuiltinSerializeImpl(const T &obj): BSIObjectWrapperSerialize<T>(obj) {}

    Result serializeImpl(StorageBuffer<> &buffer) const {
        ASSERT_ON_MSG(buffer.size() < getSizeImpl(), "Buffer size too small");
        char *s = buffer.get<char>();
        ::strncpy(s, this->getObjRef().c_str(), buffer.size());
        return Result::Success;
    }
    size_t getSizeImpl() const {
        return this->getObjRef().length() + 1;
    }
};

template<>
class BuiltinDeserializeImpl<std::string>: public BSIObjectWrapperDeserialize<std::string>{
public:
    std::string getObj() const {
        // removing trailing \0
        auto cstr = this->buffer.get<const char>();
        auto len = ::strnlen(cstr, buffer.size() - 1);
        //auto len = buffer.size() - 1;
        return std::string{cstr, len};
    }
};

/************************/

template<CSerializable F, CSerializable S>
class BuiltinSerializeImpl<std::pair<F, S>>: public BSIObjectWrapperSerialize<std::pair<F, S>>{
    using T = std::pair<F, S>;
public:
    BuiltinSerializeImpl(const T &obj): BSIObjectWrapperSerialize<T>(obj) {}

    Result serializeImpl(StorageBuffer<> &buffer) const {
        ASSERT_ON_MSG(buffer.size() < getSizeImpl(), "Buffer size too small");

        size_t offset = 0;
        const auto &obj = this->template getObjRef();
        return SerializeSequentially(buffer, offset, obj.first, obj.second);
    }
    constexpr size_t getSizeImpl() const {
        const auto &obj = this->template getObjRef();
        return szeimpl::size(obj.first) + szeimpl::size(obj.second);
    }
};

template<CSerializable F, CSerializable S>
class BuiltinDeserializeImpl<std::pair<F, S>>: public BSIObjectWrapperDeserialize<std::pair<F, S>>{
    using T = std::pair<F, S>;
public:
    T getObj() const {
        size_t offset = 0;
        StorageBufferRO buf = this->buffer;
        auto [f, s] = DeserializeSequentially<F, S>(buf, offset);

        return std::make_pair(f, s);
    }
};

/******************/

#if defined(__APPLE__)
#define REVERSE_TUPLE_APPLY  0
#define REVERSE_TUPLE_CREATE 0
#else
#define REVERSE_TUPLE_APPLY  0
#define REVERSE_TUPLE_CREATE 1
#endif

template <std::size_t ... Is>
constexpr auto indexSequenceReverse (std::index_sequence<Is...> const &)
   -> decltype( std::index_sequence<sizeof...(Is)-1U-Is...>{} );
template <std::size_t N>
using makeIndexSequenceReverse = decltype(indexSequenceReverse(std::make_index_sequence<N>{}));

template<typename T, size_t... I>
constexpr auto reverse_impl(T&& t, std::index_sequence<I...>) {
  return std::make_tuple(std::get<sizeof...(I) - 1 - I>(std::forward<T>(t))...);
}
template<typename T>
constexpr auto reverse_tuple(T&& t) {
  return reverse_impl(std::forward<T>(t), std::make_index_sequence<std::tuple_size<T>::value>());
}

template<typename Tuple, typename F, std::size_t ...I>
constexpr auto tuple_create_impl(F&& f, std::index_sequence<I...>) {
    return std::make_tuple(f.template operator()<std::tuple_element_t<I, Tuple>>()...);
}
template<typename Tuple, typename F>
constexpr auto tuple_create(F&& f) {
#if REVERSE_TUPLE_CREATE
    using Indices = makeIndexSequenceReverse<std::tuple_size<std::remove_cvref_t<Tuple>>::value>;
    return reverse_tuple(tuple_create_impl<Tuple, F>(std::forward<F>(f), Indices()));
#else
    using Indices = std::make_index_sequence<std::tuple_size<std::remove_cvref_t<Tuple>>::value>;
    return tuple_create_impl<Tuple, F>(std::forward<F>(f), Indices{});
#endif
}

template <typename Tuple, typename F, std::size_t... I>
constexpr decltype(auto) tuple_apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>) {
    // This implementation is valid since C++20 (via P1065R2)
    // In C++17, a constexpr counterpart of std::invoke is actually needed here
    return std::invoke(std::forward<F>(f), std::get<I>(std::forward<Tuple>(t))...);
}
template <typename Tuple, typename F>
constexpr decltype(auto) tuple_apply(F&& f, Tuple&& t) {
#if REVERSE_TUPLE_APPLY
    using Indices = makeIndexSequenceReverse<std::tuple_size<std::remove_cvref_t<Tuple>>::value>;
#else
    using Indices = std::make_index_sequence<std::tuple_size<std::remove_cvref_t<Tuple>>::value>;
#endif
    return tuple_apply_impl(std::forward<F>(f), std::forward<Tuple>(t), Indices{});
}

template<typename U, typename T, size_t... I>
static U instantiate_from_tuple_impl(T &&t, std::index_sequence<I...>){
    return U{std::get<I>(std::forward<T>(t))...};
}
template<typename U, typename T, size_t... I>
static U instantiate_from_tuple(T &&t){
    return instantiate_from_tuple_impl<U>(std::forward<T>(t), std::make_index_sequence<std::tuple_size<T>::value>());
}

/******************/

template<CSerializable ...TArgs>
class BuiltinSerializeImpl<std::tuple<TArgs...>>: public BSIObjectWrapperSerialize<std::tuple<TArgs...>>{
    using T = std::tuple<TArgs...>;
public:
    BuiltinSerializeImpl(const T &obj): BSIObjectWrapperSerialize<T>(obj) {}

    Result serializeImpl(StorageBuffer<> &buffer) const {
        ASSERT_ON_MSG(buffer.size() < getSizeImpl(), "Buffer size too small");
        Result res = Result::Success;
        size_t offset = 0;
        const auto &obj = this->template getObjRef();
        
        StorageBuffer buf;
        tuple_apply([&buf, buffer, &offset, &res](auto&&... arg){
            ((
                res = SerializeSequentially(buffer, offset, arg) //std::remove_cvref_t<decltype(arg)>
            ), ...);
        }, obj);
        return res;
    }

    constexpr size_t getSizeImpl() const {
        size_t sum = 0;
        std::apply([&sum](auto&&... arg){
            ((sum += szeimpl::size(arg)),
            ...);
        }, this->template getObjRef());
        return sum;
    }
};

template<CSerializable ...TArgs>
class BuiltinDeserializeImpl<std::tuple<TArgs...>>: public BSIObjectWrapperDeserialize<std::tuple<TArgs...>>{
    using T = std::tuple<TArgs...>;
public:
    T getObj() const {
        size_t offset = 0;
        StorageBufferRO buf = this->buffer;
        auto deserialize_argument = [&buf, &offset]<typename T>() -> T{
            auto [t] = DeserializeSequentially<T>(buf, offset);
            return t;
        };
        return tuple_create<T>(deserialize_argument);
    }
};

/*****************************/

template<CSerializable T>
class BuiltinSerializeImpl<std::unique_ptr<T>>{
public:
    BuiltinSerializeImpl(const std::unique_ptr<T> &obj) {}
    Result serializeImpl(StorageBuffer<> &buffer) const {
        return Result::Success;
    }
    constexpr size_t getSizeImpl() const {
        return 0;
    }
};

template<CSerializable T>
class BuiltinDeserializeImpl<std::unique_ptr<T>>: BSIObjectWrapperDeserialize<std::unique_ptr<T>>{
public:
    std::unique_ptr<T> getObj(const StorageBufferRO<> &) const{
        return std::unique_ptr<T>{};
    }
};


/************************/

template<>
class BuiltinSerializeImpl<std::vector<bool>>: public BSIObjectWrapperSerialize<std::vector<bool>>{
    using T = std::vector<bool>;
    using StoredType = uint64_t;
    static constexpr size_t StoredSize = sizeof(StoredType) * 8;
public:
    BuiltinSerializeImpl(const T &obj): BSIObjectWrapperSerialize<T>(obj) {}

    Result serializeImpl(StorageBuffer<> &buffer) const {
        ASSERT_ON_MSG(buffer.size() < getSizeImpl(), "Buffer size too small");
        Result res = Result::Success;
        size_t offset = 0;
        const auto &obj = this->getObjRef();
        StorageBuffer buf;

        size_t size = obj.size();
        size += StoredSize * !!(size % StoredSize) - (size % StoredSize);
        res = SerializeSequentially(buffer, offset, size);
        
        size = obj.size();
        for(size_t i = 0; i < size; i += StoredSize){
            StoredType val = 0;
            size_t num_bits = StoredSize;
            if (size - i < StoredSize){
                num_bits = size - i;
            }
            for(size_t j = 0; j < num_bits; j++){
                if (obj[i + j]){
                    val |= (1ULL << j);
                }
            }
            res = SerializeSequentially(buffer, offset, val);
        }

        return res;
    }
    size_t getSizeImpl() const {
        const auto &obj = this->getObjRef();
        size_t size = obj.size();
        size = size / StoredSize + !!(size % StoredSize);
        return szeimpl::size(size) + szeimpl::size(StoredType{}) * size;
    }
};

template<>
class BuiltinDeserializeImpl<std::vector<bool>>: public BSIObjectWrapperDeserialize<std::vector<bool>>{
    using T = std::vector<bool>;
    using StoredType = uint64_t;
    static constexpr size_t StoredSize = sizeof(StoredType) * 8;
public:
    T getObj() const {
        size_t offset = 0;
        StorageBufferRO buf = this->buffer;
        T vec;

        auto [size] = DeserializeSequentially<size_t>(buf, offset);
        vec.resize(size, false);
        
        for(size_t i = 0; i < size; i += StoredSize){
            auto [chunk] = DeserializeSequentially<StoredType>(buf, offset);
            for(size_t j = 0; j < StoredSize; j++){
                if(chunk & (1ULL << j)){
                    vec[i + j] = true;
                }
            }
        }

        return vec;
    }
};

template<typename T>
requires CSerializableRange<T>
class BuiltinSerializeImpl<T>: public BSIObjectWrapperSerialize<T>{
    using U = std::ranges::range_value_t<T>;
public:
    BuiltinSerializeImpl(const T &obj): BSIObjectWrapperSerialize<T>(obj) {}

    Result serializeImpl(StorageBuffer<> &buffer) const {
        Result res = Result::Success;
        ASSERT_ON_MSG(buffer.size() < getSizeImpl(), "Buffer size too small");

        size_t offset = 0;
        const auto &obj = this->template getObjRef();

        auto size = std::ranges::size(obj);
        res = SerializeSequentially(buffer, offset, size);

        auto ser = [&](const U &u){
            res = SerializeSequentially(buffer, offset, u);
        };
        std::ranges::for_each(obj, ser);

        return res;
    }
    size_t getSizeImpl() const {
        const auto &obj = this->template getObjRef();

        auto size = szeimpl::size(std::ranges::size(obj));
        auto sum = [&size](size_t s){ size += s; };
        std::ranges::for_each(obj | std::views::transform(szeimpl::size<U>), sum); //std::as_const?

        return size;
    }
};

template<typename T>
struct EmplaceWrapper{
};

template<typename T, typename... Args>
concept has_absolute_emplace = requires(T& t, Args&&... args) {
    t.emplace(std::forward<Args>(args)...);
};

template<typename T, typename... Args>
concept has_positional_emplace = requires(T& t, T::const_iterator it, Args&&... args) {
    t.emplace(it, std::forward<Args>(args)...);
};



template<typename T, typename... Args>
requires has_absolute_emplace<T, Args...>
void wrapped_emplace(T &t, Args&&... args){
    t.emplace(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
requires has_positional_emplace<T, Args...> && std::ranges::range<T> && (!has_absolute_emplace<T, Args...>)
void wrapped_emplace(T &t, Args&&... args){
    t.emplace(std::ranges::end(t), std::forward<Args>(args)...);
}

template<typename T>
requires sized_forward_range<T> && CSerializable<std::ranges::range_value_t<T>>
class BuiltinDeserializeImpl<T>: public BSIObjectWrapperDeserialize<T>{
    using U = std::ranges::range_value_t<T>;
public:
    T getObj() const {
        size_t offset = 0;
        StorageBufferRO buf = this->buffer;
        T r;

        auto [size] = DeserializeSequentially<std::ranges::range_size_t<T>>(buf, offset);
        
        for(size_t i = 0; i < size; i++){
            auto [u] = DeserializeSequentially<U>(buf, offset);
            wrapped_emplace(r, std::move(u));
        }

        return r;
    }
};
