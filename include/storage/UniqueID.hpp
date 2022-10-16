#pragma once

#include <type_traits>
#include <tuple>
#include <cstdint>
#include <map>
#include <memory>
#include <functional>

#include <storage/StorageUtils.hpp>
#include <storage/SerializeImpl.hpp>
#include <storage/UniqueIDInterface.hpp>
#include <storage/DataStorage.hpp>

class DataStorageBase;

/************************************************************************/

template<typename T, CStorage Storage, UniqueIDName IDName = UniqueIDName::Instance>
requires CUniqueID<T, IDName>
class UniqueIDStorage {
    Storage &storage;
    std::map<uint32_t, std::pair<StorageAddress, std::unique_ptr<T>>> m;
    //std::map<std::string, uint32_t> types;
    //TODO use typeid(variable).name() to store actual type and perform type check
    uint32_t max_id{UniqueIDInterface<IDName>::DEFAULT};
public:
    static constexpr size_t UNKNOWN_ID = 0;
    UniqueIDStorage(Storage &storage): storage(storage) {}

    void registerInstance(const UniqueIDInterface<IDName> &u){
        T *t_ptr = static_cast<T *>(&(const_cast<std::add_lvalue_reference_t<std::remove_const_t<std::remove_reference_t<decltype(u)>>>>(u)));
        ASSERT_ON(t_ptr == nullptr);
        auto id = u.UniqueIDInstance::getUniqueID();
        auto it = m.find(id);
        if(it != m.end()){
            ASSERT_ON(it->second.second.get() != nullptr);
            it->second.second.reset(t_ptr);
        } else {
            m.emplace(id, std::make_pair(StorageAddress{}, std::unique_ptr<T>(t_ptr)));
        }
    }
    template<CUniqueID<IDName> U> //requires derived 
    U &getInstance(const UniqueIDInterface<IDName> &u){
        auto id = u.UniqueIDInstance::getUniqueID();
        auto it = m.find(id);
        ASSERT_ON(it == m.end());
        auto &t_ptr = it->second.second;
        if(t_ptr.get() == nullptr){
            //read it from address
            auto addr = it->second.first;
            ASSERT_ON(addr.is_null());
            auto new_ptr = deserialize_ptr<U>(storage, addr, id);
            t_ptr.swap(new_ptr);
        }
        return *t_ptr.get();
    }
    void registerInstanceAddress(const UniqueIDInterface<IDName> &u, const StorageAddress &addr){
        auto id = u.UniqueIDInstance::getUniqueID();
        auto it = m.find(id);
        ASSERT_ON(it == m.end());
        it->second.first = addr;
    }
    void deleteInstance(const UniqueIDInterface<IDName> &u){
        m.erase(u.getUniqueID());
    }
    uint32_t generateID(){
        ASSERT_ON(max_id == std::numeric_limits<decltype(max_id)>::max());
        return ++max_id;
    }

    using RemovedUPTR = std::pair<uint32_t, StorageAddress>;
    RemovedUPTR remove_uptr(const typename decltype(m)::value_type &p) const{
        return {p.first, p.second.first};
    }

    Result serializeImpl(StorageBuffer<> &buffer) const {
        ASSERT_ON(getSizeImpl() > buffer.allocated());
        size_t offset = 0;

        auto buf = buffer.offset_advance(offset, szeimpl::size(max_id));
        szeimpl::s(max_id, buf);

        auto m_size = m.size();
        buf = buffer.offset_advance(offset, szeimpl::size(m_size));
        szeimpl::s(m_size, buf);

        for(const auto &it: m){
            auto it_r_uptr = remove_uptr(it);
            buf = buffer.offset_advance(offset, szeimpl::size(it_r_uptr));
            szeimpl::s(it_r_uptr, buf);
        }
        return Result::Success;
    }
    static UniqueIDStorage<T, Storage> deserializeImpl(const StorageBufferRO<> &buffer, Storage &storage) {
        UniqueIDStorage<T, Storage> uid{storage};

        size_t offset = 0;
        auto buf = buffer;
        uid.max_id = szeimpl::d<decltype(uid.max_id)>(buf);
        buf = buffer.advance_offset(offset, szeimpl::size(uid.max_id));

        auto num = szeimpl::d<typename decltype(m)::size_type>(buf);
        buf = buffer.advance_offset(offset, szeimpl::size(num));
        for(size_t i = 0; i < num; i++){
            auto val = szeimpl::d<RemovedUPTR>(buf);
            buf = buffer.advance_offset(offset, szeimpl::size(val));
            uid.m.emplace(val.first, std::make_pair(val.second, std::unique_ptr<T>{}));
        }
        return uid;
    }
    static UniqueIDStorage<T, Storage> deserializeImpl(const StorageBufferRO<> &buffer) { throw std::bad_function_call("Not implemented"); };
    size_t getSizeImpl() const {
        typename decltype(m)::size_type s{0};
        size_t sum = szeimpl::size(max_id) + szeimpl::size(s);
        for(const auto &e: m){
            auto e_r_uptr = remove_uptr(e);
            sum += szeimpl::size(e_r_uptr);
        }
        return sum;
    }
};

template<typename T, UniqueIDName IDName = UniqueIDName::Instance>
requires CUniqueID<T, IDName>
class UniqueIDPtr{
    T *ptr{nullptr};
    UniqueIDInterface<IDName> uid;
public:
    UniqueIDPtr() {}
    UniqueIDPtr(T *ptr): ptr(ptr), uid(*ptr) {}
    UniqueIDPtr(T &ptr): ptr(&ptr), uid(ptr) {}
    UniqueIDPtr(UniqueIDInterface<IDName> &uid): uid(uid) {}

    template<typename U = T>
    U *getPtr() const{
        return static_cast<U *>(ptr);
    }
    template<typename U = T>
    U &get() const{
        return *getPtr<U>();
    }
    bool is_init() const{
        return ptr != nullptr;
    }
    bool is_empty() const{
        return ptr == nullptr && uid.getUniqueID() == UniqueIDInterface<IDName>::DEFAULT;
    }
    uint32_t getID() const{
        if(is_init()){
            return ptr->UniqueIDInterface<IDName>::getUniqueID();
        } else {
            return uid.getUniqueID();
        }
    }
    template<typename U, CStorage Storage>
    requires std::is_base_of_v<T, U>
    void init(UniqueIDStorage<U, Storage, IDName> &uid_storage){
        if(is_init() || is_empty()){
            return;
        }
        ptr = dynamic_cast<U *>(&uid_storage.template getInstance<U>(uid));
    }

    Result serializeImpl(StorageBuffer<> &buffer) const {
        if(is_init()){
            return szeimpl::s(*static_cast<UniqueIDInterface<IDName> *>(ptr), buffer);
        } else {
            return szeimpl::s(uid, buffer);
        }
    }
    template<CStorage Storage>
    static UniqueIDPtr<T, IDName> deserializeImpl(const StorageBufferRO<> &buffer, UniqueIDStorage<T, Storage, IDName> &uid_storage) {
        auto uid = szeimpl::d<UniqueIDInterface<IDName>>(buffer);
        return UniqueIDPtr<T, IDName>{uid_storage.template getInstance<T>(uid.getUniqueID())};
    }
    static UniqueIDPtr<T, IDName> deserializeImpl(const StorageBufferRO<> &buffer) {
        auto uid = szeimpl::d<UniqueIDInterface<IDName>>(buffer);
        return UniqueIDPtr<T, IDName>{uid};
    }
    size_t getSizeImpl() const {
        return szeimpl::size(*static_cast<UniqueIDInterface<IDName> *>(ptr));
    }
};
