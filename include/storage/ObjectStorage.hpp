#pragma once

#include <storage/Utils.hpp>
#include <storage/StorageUtils.hpp>
#include <storage/SerializeImpl.hpp>
#include <storage/DataStorage.hpp>


template<typename F, typename T>
concept COSForEachCallable = requires(F &f, T &t, size_t index){
    { f(index, t) } -> std::same_as<void>;
    //requires std::invocable<F, T &>;
};

template<typename F, typename T>
concept COSForEachEmptyCallable = requires(F &f, size_t index){
    { f(index) } -> std::same_as<T>;
};

template<typename T>
using OSForEachCallableFunction = std::function<void(size_t, T &)>;

template<typename T>
using OSForEachEmptyCallableFunction = std::function<T(size_t)>;

template<typename T, typename U, typename Storage, size_t MAX_OBJS>
    //ObjectStorageAccess Access, typename F, typename FC, typename FI>//, typename R>
concept CObjectStorage = requires(const T &t, T &t2, const U &u, const U &u2,
        size_t index, size_t index2, Storage &storage,
        const StorageAddress &base,
        OSForEachCallableFunction<U> &f,
        OSForEachCallableFunction<const U> &fc,
        OSForEachEmptyCallableFunction<U> &fi){
    { T(storage, base) };
    { T(storage) };
    { t.has(index) } -> std::same_as<bool>;
    { t2.template get(index) } -> std::same_as<U>;
    //{ t2.get(index, index2) } -> std::same_as<R>;
    { t2.template getRef(index) } -> std::same_as<U&>;
    { t2.template getCRef(index) } -> std::same_as<const U&>;
    { t2.clear(index, index2) } -> std::same_as<void>;
    { t.size() } -> std::same_as<size_t>;
    { t2.template put(u) } -> std::same_as<size_t>;
    { t2.template put(u, index) } -> std::same_as<void>;
    { t2.template put(u2) } -> std::same_as<size_t>;
    { t2.template put(u2, index) } -> std::same_as<void>;
    { t2.template for_each(index, index2, f) } -> std::same_as<void>;
    //{ t2.template for_each(index, index2, fc) } -> std::same_as<void>;
    { t2.template for_each_empty(index, index2, fi) } -> std::same_as<size_t>;
    requires CSerializable<T>;
    requires CSerializable<U>;
    requires CStorage<Storage>;
    //requires CSerializableRange<R, U>;
    //requires COSForEachCallable<F, T>;
    //requires COSForEachCallable<FC, const T>;
    //requires COSForEachIndexCallable<FI, T>;
};

constexpr uint32_t ObjectStorageMagic = 0x0B13C787;

template<CSerializable T, template<typename, typename, size_t> typename ObjectStorageImpl,
    CStorage Storage, size_t MAX_OBJS = std::numeric_limits<size_t>::max()>
requires CObjectStorage<ObjectStorageImpl<T, Storage, MAX_OBJS>, T, Storage, MAX_OBJS>
using ObjectStorage = AutoStoredObject<TypedObject<ObjectStorageImpl<T, Storage, MAX_OBJS>, ObjectStorageMagic>, Storage, (1UL << 20)>;


//TODO ObjexctStorage.cpp to test basic functionaliy without any assumptions on object origin and storage format.
//why not generic tests for all? some OS would have implementation specialized to access or storage format
