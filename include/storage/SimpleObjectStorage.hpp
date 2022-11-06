#pragma once

#include <storage/Utils.hpp>
#include <storage/StorageUtils.hpp>
#include <storage/StorageHelpers.hpp>
#include <storage/DataStorage.hpp>
#include <storage/SerializeImpl.hpp>
#include <storage/Serialize.hpp>


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
    SimpleObjectStorage(Storage &storage, StorageAddress base):
        storage(storage), base(base), tail_addr(base), count{0} {}
    static constexpr ObjectStorageAccess DefaultAccess = ObjectStorageAccess::Once;
    bool has(size_t index) const {
        if (index >= states.size()){
            return false;
        }
        return states[index];
    }
    size_t size() const {
        return count;
    }
    T get(size_t index, ObjectStorageAccess access){
        ASSERT_ON(!has(index));
        auto it = objects.find(index);
        if (it != objects.end()){
            return it->second;
        }
        auto addr = getAddrByIndex(index);
        auto t = deserialize<T>(storage, addr);
        switch(access){
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
    const T &getCRef(size_t index){
        return getRef(index);
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
    void put(const T &t, size_t index){
        objects.erase(index);
        //there could be some object with this index already
        putImpl(t, index, true);
    }
    size_t put(const T &t){
        size_t index = getNewIndex();
        putImpl(t, index, false);
        return index;
    }
    void put(T &&t, size_t index, ObjectStorageAccess access){
        objects.erase(index);
        putImpl(t, index, true);
        if(access == ObjectStorageAccess::Keep){
            objects.emplace(index, std::forward<T>(t));
        }
    }
    size_t put(T &&t, ObjectStorageAccess access){
        size_t index = getNewIndex();
        put(std::forward<T>(t), index, access);
        return index;
    }

    /* Serialize */

    Result serializeImpl(StorageBuffer<> &buffer) const {
        Result res = Result::Success;
        size_t offset = 0;
        StorageBuffer buf;
    
        buf = buffer.offset_advance(offset, szeimpl::size(base));
        res = szeimpl::s(base, buf);
    
        buf = buffer.offset_advance(offset, szeimpl::size(states));
        res = szeimpl::s(states, buf);
    
        buf = buffer.offset_advance(offset, szeimpl::size(addresses));
        res = szeimpl::s(addresses, buf);
    
        buf = buffer.offset_advance(offset, szeimpl::size(unused_addresses));
        res = szeimpl::s(unused_addresses, buf);
        return res;
    }
    static SimpleObjectStorage<T, Storage> deserializeImpl(const StorageBufferRO<> &buffer, Storage &storage) {
        size_t offset = 0;
        auto buf = buffer;

        StorageAddress base = szeimpl::d<StorageAddress>(buf);
        buf = buffer.advance_offset(offset, szeimpl::size(base));

        auto states = szeimpl::d<std::vector<bool>>(buf);
        buf = buffer.advance_offset(offset, szeimpl::size(states));

        auto addresses = szeimpl::d<std::unordered_map<size_t, StorageAddress>>(buf);
        buf = buffer.advance_offset(offset, szeimpl::size(addresses));

        auto unused_addresses = szeimpl::d<SetOrderedBySize<StorageAddress>>(buf);

        return SimpleObjectStorage<T, Storage>{storage, base, std::move(states), std::move(addresses), std::move(unused_addresses)};
    }
    static SimpleObjectStorage<T, Storage> deserializeImpl(const StorageBufferRO<> &) { throw std::bad_function_call(); };
    size_t getSizeImpl() const {
        return szeimpl::size(base) + szeimpl::size(states) + szeimpl::size(addresses) + szeimpl::size(unused_addresses);
    }
};
