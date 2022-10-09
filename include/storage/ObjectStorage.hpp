#pragma once

#include <storage/Utils.hpp>
#include <storage/StorageUtils.hpp>


template<typename T, typename U>
concept CObjectStorageImpl = requires(const T &t, T &t2, const U &u, const U &u2, size_t index, size_t index2, ObjectStorageAccess access){
    { t.has(index) } -> std::same_as<bool>;
    { t2.get(index, access) } -> std::same_as<U>;
    { t2.getRef(index) } -> std::same_as<U&>;
    { t2.getCRef(index) } -> std::same_as<const U&>;
    { t2.clear(index, index2) } -> std::same_as<void>;
    { t.size() } -> std::same_as<size_t>;
    { t2.put(u) } -> std::same_as<size_t>;
    { t2.put(u, index) } -> std::same_as<void>;
    { t2.put(u2, access) } -> std::same_as<size_t>;
    { t2.put(u2, index, access) } -> std::same_as<void>;
    requires CSerializable<T>;
};

template<CSerializable T, CStorageImpl Storage, typename ObjectStorageImpl, size_t MAX_OBJS = std::numeric_limits<size_t>::max()>
requires CObjectStorageImpl<T>
class ObjectStorage{
    Storage &storage;
    StaticHeader<T> header;
    ObjectStorageImpl<T, MAX_OBJS> obj_storage_impl;

    static constexpr uint32_t OS_MAGIC = 0xF4E570F3;
    static constexpr size_t DEFAULT_ALLOC_SIZE = (1 << 32); //4GB

    void init_objstorage(){
        ASSERT_ON(header.size < szeimpl::size(StaticHeader<T>{}));
        if(header.magic != OS_MAGIC){
            //new
            header.magic = OS_MAGIC;
            header.address = storage.template get_random_address(DEFAULT_ALLOC_SIZE);
        } else {
            //deserialize
            obj_storage_impl = deserialize<ObjectStorageImpl<T, Storage, MAX_OBJS>>(storage, header.address);
        }
    }
public:
    ObjectStorage(Storage &storage, StorageAddress &addr): 
        storage(storage),
        header_address(addr),
        header(deserialize<StaticHeader<T>>(storage, header_address)) {
            init_objstorage();
    }

    //create all objects [index_start, index_finish]
    void prefetch(size_t index_start, size_t index_finish, ObjectStorageAccess access = ObjectStorageImpl::DefaultAccess){
        for(size_t i = index_start; i <= index_finish; i++){
            if(obj_storage_impl.has(i))
                obj_storage_impl.get(i, access);
        }
    }
    void prefetch_all(ObjectStorageAccess access = ObjectStorageImpl::DefaultAccess){
        if(obj_count > 0)
            prefetch(0, obj_count - 1, access);
    }
    //delete all objects from
    void clear(size_t index_start, size_t index_finish){
        obj_storage_impl.clear(index_start, index_finish);
    }
    void clear_all(){
        if(obj_count > 0)
            clear(0, obj_count - 1);
    }

    //return object deserialized from that address
    template<typename U = T>
    U getVal(size_t index, ObjectStorageAccess access = ObjectStorageImpl::DefaultAccess){
        ASSERT_ON(!obj_storage_impl.has(index));
        return static_cast<U>(obj_storage_impl.get(index, access));
    }

    //get ref to object, owned by Object Storage
    template<typename U = T>
    U &getRef(size_t index, ObjectStorageAccess access = ObjectStorageImpl::DefaultAccess){
        ASSERT_ON(!obj_storage_impl.has(index));
        return static_cast<U &>(obj_storage_impl.get(index, access));
    }

    //serialize object, return index
    template<typename ...Args>
    size_t put(const T &t, Args&&... args){
        return obj_storage_impl.put(std::forward<T>(t), std::forward<Args>(args)...);
    }

    //iterate
    template<typename F, typename R = void>
    requires std::invocable<F, T>
    R for_each(size_t index_start, size_t index_finish, F &f,
            ObjectStorageAccess access = ObjectStorageImpl::DefaultAccess){
        for(size_t i = index_start; i <= index_finish; i++){
            if(obj_storage_impl.has(i))
                f(obj_storage_impl.getRef(i, access));
        }
    }

    //serialize object, return index
    size_t put(T &&t){
        return obj_storage_impl.put(std::forward<T>(t));
    }

    //serialize object at index
    void put(const T &t, size_t index){
        ASSERT_ON(index >= MAX_OBJS);
        obj_storage_impl.put(std::forward<T>(t), index);
    }
    void put(T &&t, size_t index){
        ASSERT_ON(index >= MAX_OBJS);
        obj_storage_impl.put(std::forward<T>(t), index);
    }

    size_t size() const{
        return obj_count;
    }
};
