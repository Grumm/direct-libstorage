#pragma once

#include <type_traits>
#include <tuple>
#include <cstdint>
#include <map>
#include <memory>

#include <storage/StorageUtils.hpp>
#include <storage/SerializeImpl.hpp>
#include <storage/StorageManagerDecl.hpp>

enum class UniqueIDName{
    Instance,
    Type,
};
template<UniqueIDName I>
class UniqueIDInterface{
    uint32_t id;
public:
    UniqueIDInterface(uint32_t id): id(id) {}
    uint32_t getUniqueID() const { return id; }
};

using UniqueIDInstance = UniqueIDInterface<UniqueIDName::Instance>;
using UniqueIDType = UniqueIDInterface<UniqueIDName::Type>;

class DataStorage;

/************************************************************************/

template<typename T>
class UniqueIDStorage {
    DataStorage &storage;
    std::map<uint32_t, std::pair<StorageAddress, std::unique_ptr<T>>> m;
    uint32_t max_id{1};
public:
    UniqueIDStorage(DataStorage &storage): storage(storage) {}

    template<typename U = T>
    void registerUniqueInstance(const U &u){
        T *t_ptr = dynamic_cast<T *>(const_cast<std::remove_const_t<U *>>(&u));
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
    template<typename U> //requires derived 
    U &getUniqueInstance(const UniqueIDInstance &instance) const{
        auto id = instance.getUniqueID();
        auto it = m.find(id);
        ASSERT_ON(it == m.end());
        auto &t_ptr = it->second.second;
        if(t_ptr.get() == nullptr){
            //read it from address
            auto addr = it->second.first;
            ASSERT_ON(addr.is_null());
            auto new_ptr = deserialize_ptr<U>(storage, addr);
            t_ptr.swap(new_ptr);
        }
        return *t_ptr.get();
    }
    uint32_t generateID(){
        ASSERT_ON(max_id == std::numeric_limits<decltype(max_id)>::max());
        return ++max_id;
    }

    using RemovedUPTR = std::pair<uint32_t, StorageAddress>;
    RemovedUPTR remove_uptr(const typename decltype(m)::value_type &p) const{
        return {p.first, p.second.first};
    }
    //TODO serialize impl
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
    }
    static UniqueIDStorage<T> deserializeImpl(const StorageBufferRO<> &buffer) {
        UniqueIDStorage<T> uid{GetGlobalMetadataStorage()}; //GetStorageManager().getStorage()

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