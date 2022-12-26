#pragma once

#include <storage/Utils.hpp>
#include <storage/StorageUtils.hpp>
#include <storage/StorageHelpers.hpp>
#include <storage/DataStorage.hpp>
#include <storage/SerializeImpl.hpp>
#include <storage/Serialize.hpp>
#include <storage/ObjectStorage.hpp>


//Implements CObjectStorageImpl
//Can store any object. No matter the sze::size() of T
template<typename T, typename Storage, size_t MAX_OBJS = std::numeric_limits<size_t>::max()>
requires CStorage<Storage, T>
class SimpleObjectStorage{
    Storage &storage;
    StorageAddress base;
    StorageAddress tail_addr;

    struct ObjState{
        T obj;
        ObjectStorageAccess access;
    };
    std::unordered_map<size_t, T> objects;
    std::vector<bool> states;
    std::unordered_map<size_t, StorageAddress> addresses;
    SetOrderedBySize<StorageAddress> unused_addresses;
    size_t count;
    //max_obj_index

    StorageAddress getNewAddress(size_t size) {
        auto advance_address = [](StorageAddress &addr, size_t size) -> StorageAddress{
            auto new_addr = addr.subrange(0, size);
            addr = addr.offset(size);
            return new_addr;
        };
        auto [has, address] = unused_addresses.splice(size, advance_address);
        if(has){
            return address;
        }
        StorageAddress new_addr = tail_addr.subrange(0, size);
        tail_addr = tail_addr.offset(size);
        return new_addr;
    }
    StorageAddress getAddrByIndex(size_t index){
        auto it = addresses.find(index);
        ASSERT_ON(it == addresses.end());
        return it->second;
    }
    void allocAddr(size_t index, const StorageAddress &addr){
        addresses.emplace(index, addr);
    }
    void delAddr(size_t index){
        addresses.erase(index);
    }
    StorageAddress allocAddrForIndex(size_t index, size_t size){
        auto it = addresses.find(index);
        if(it != addresses.end()){
            auto old_addr = it->second;
            if(size < old_addr.size){
                return old_addr;
            }
            //reallocate old_addr
            delAddr(index);
        }
        //allocate new address
        StorageAddress addr = getNewAddress(size); //TODO how do we know address without deserializing first?
        allocAddr(index, addr);
        return addr;
    }
    void allocState(size_t index){
        ASSERT_ON(index >= MAX_OBJS);
        if(index >= states.size()){
            states.resize(index + 1, false);
        }
    }
    void setState(size_t index, bool val){
        allocState(index);
        states[index] = val;
    }
    size_t getNewIndex(){
        ASSERT_ON(count >= MAX_OBJS);
        ASSERT_ON(states.size() >= MAX_OBJS);
        size_t index = states.size();
        allocState(index);
        return index;
    }
    
    void putImpl(const T &t, size_t index, bool check_index){
        if((check_index && !has(index)) || !check_index){
            count++;
        }
        setState(index, true);
        size_t size = sze::getSize<T>(t);
        auto addr = allocAddrForIndex(index, size);
        serialize<StorageAddress>(storage, t, addr);
        //no need to keep in .object
    }
    SimpleObjectStorage(Storage &storage, StorageAddress base, std::vector<bool> &&states,
                        std::unordered_map<size_t, StorageAddress> addresses, SetOrderedBySize<StorageAddress> unused_addresses):
        storage(storage), base(base), states(states), 
        addresses(addresses), unused_addresses(unused_addresses), count{addresses.size()} {}
public:
    SimpleObjectStorage(Storage &storage, const StorageAddress &base):
        storage(storage), base(base), tail_addr(base), count{0} {}
    SimpleObjectStorage(Storage &storage):
        SimpleObjectStorage(storage, storage.template get_random_address(DEFAULT_ALLOC_SIZE)) {}

    static constexpr ObjectStorageAccess DefaultAccess = ObjectStorageAccess::Once;
    static constexpr size_t DEFAULT_ALLOC_SIZE = (1UL << 20);
    bool has(size_t index) const {
        if (index >= states.size()){
            return false;
        }
        return states[index];
    }
    size_t size() const {
        return count;
    }
    
    template <ObjectStorageAccess Access = DefaultAccess>
    T get(size_t index){
        ASSERT_ON(!has(index));
        auto it = objects.find(index);
        if (it != objects.end()){
            return it->second;
        }
        auto addr = getAddrByIndex(index);
        auto t = deserialize<T>(storage, addr);
        switch(Access){
            case ObjectStorageAccess::Once:
            case ObjectStorageAccess::Caching:
                break;
            case ObjectStorageAccess::Keep:
                count++;
                setState(index, true);
                objects.emplace(index, t);
                break;
        }
        return t;
    }
    //implies Keep
    template <ObjectStorageAccess Access = DefaultAccess>
    T &getRef(size_t index){
        ASSERT_ON(!has(index));
        auto it = objects.find(index);
        if (it != objects.end()){
            return it->second;
        }
        count++;
        setState(index, true);
        auto addr = getAddrByIndex(index);
        auto t = deserialize<T>(storage, addr);
        auto [it2, emplaced] = objects.emplace(index, t); //ideally we should add it to tmp_objects. question would be it's lifetime
        //TODO throw if !emplaced
        return it2->second;
    }
    template <ObjectStorageAccess Access = DefaultAccess>
    const T &getCRef(size_t index){
        return getRef<Access>(index);
    }

    void clear(size_t i){
        if(!has(i)){
            return;
        }
        size_t count_erased = objects.erase(i);
        count -= count_erased;
        setState(i, false);
        auto it = addresses.find(i);
        unused_addresses.add(it->second.size, it->second);
        addresses.erase(it);
    }
    void clear(size_t start, size_t end){
        for(size_t i = start; i <= end; i++){
            clear(i);
        }
    }
    template <ObjectStorageAccess Access = DefaultAccess>
    size_t put(const T &t){
        size_t index = getNewIndex();
        put<Access>(std::forward<const T &>(t), index);
        return index;
    }
    template <ObjectStorageAccess Access = DefaultAccess>
    void put(const T &t, size_t index){
        objects.erase(index);
        putImpl(t, index, true);
        if(Access == ObjectStorageAccess::Keep){
            objects.emplace(index, std::forward<const T &>(t));
        }
    }
    /*
    template <ObjectStorageAccess Access = DefaultAccess>
    size_t put(T &&t){
        size_t index = getNewIndex();
        put<Access>(std::forward<T>(t), index);
        return index;
    }
    template <ObjectStorageAccess Access = DefaultAccess>
    void put(T &&t, size_t index){
        objects.erase(index);
        putImpl(t, index, true);
        if(Access == ObjectStorageAccess::Keep){
            objects.emplace(index, std::forward<T &&>(t));
        }
    }*/

    template<typename F>
    requires COSForEachCallable<F, T>
    void for_each(size_t start, size_t end, F &f){
        for(size_t i = start; i <= end; i++){
            if(!has(i))
                continue;
            f(i, getRef(i));
        }
    }
    template<typename F>
    requires COSForEachCallable<F, const T>
    void for_each(size_t start, size_t end, F &f){
        for(size_t i = start; i <= end; i++){
            if(!has(i))
                continue;
            f(i, getCRef(i));
        }
    }
    template<typename F>
    requires COSForEachEmptyCallable<F, T>
    size_t for_each_empty(size_t start, size_t end, F &f){
        size_t count = 0;
        for(size_t i = start; i <= end; i++){
            if(has(i))
                continue;
            count++;

            auto newobj = f(i);
            put(std::move(newobj), i);
        }
        return count;
    }

    /* Serialize */

    Result serializeImpl(StorageBuffer<> &buffer) const {
        Result res = Result::Success;
        size_t offset = 0;

        return SerializeSequentially(buffer, offset, base, states, addresses, unused_addresses);
    }
    static SimpleObjectStorage deserializeImpl(const StorageBufferRO<> &buffer, Storage &storage) {
        size_t offset = 0;
        auto buf = buffer;
        auto [base, states, addresses, unused_addresses] = DeserializeSequentially<
                StorageAddress, std::vector<bool>, std::unordered_map<size_t, StorageAddress>, SetOrderedBySize<StorageAddress>
            >(buf, offset);

        return SimpleObjectStorage<T, Storage>{storage, base, std::move(states), std::move(addresses), std::move(unused_addresses)};
    }
    static SimpleObjectStorage deserializeImpl(const StorageBufferRO<> &) { throw std::bad_function_call(); };

    size_t getSizeImpl() const {
        return SizeAccumulate(base, states, addresses, unused_addresses);
    }
};
