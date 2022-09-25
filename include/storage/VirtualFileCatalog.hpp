#pragma once

/*
i. Interface
1) KEY -> KEY
KEY -> address
2) tuple<N...>
3) tag

DB table

What we want:
<Exchange, Ticker, Resolution, Expiration, UniqueKey> -> address

Collection<T> get_list<Ticker>(Filter)

Collection<T> create(Exchange, Ticker, Resolution, Expiration, UniqueKey);

ii. What to store?
1) Address
2) DataStorage:Address
3) VirtualFile, which has same interface as ObjectStorage
 
*/

/*
AbstractVirtualFileEntity
VirtualFileCatalog: AbstractVirtualFileEntity
VirtualFile: AbstractVirtualFileEntity

VirtualFileCatalog -> key: AbstractVirtualFileEntity

virtual file system
Folder or File name = key

*/

//TODO StaticObject which maps string to StorageAddress and VFC path


/*
Purpose: assign keys to storage addresses and able to look up between them
High-level description: multilevel key-value mapping, DB-like, serializable
key - anything in template argument,
value - DataStorage, StorageAddress, ObjectAddress, e.g. anything in template argument

key examples: Exchange, Ticker, Resolution, Expiration, UniqueKey

commands: lookup (1 or N) key and return either
- all values present of the next key, i.e. folder. no keys = root folder of first key
- or entry if no next key

when deserializing, need to match serialized type and new type.
for this we need to keep a string description of the objects and compare to this
this is a part of objectstorage class

*/

#include <type_traits>
#include <typeinfo>
#include <string>
#include <map>

#include <storage/DataStorage.hpp>
#include <storage/Serialize.hpp>
#include <storage/RandomMemoryAccess.hpp>
#include <storage/SimpleStorage.hpp>
#include <storage/StorageManager.hpp>
#include <storage/StorageManagerDecl.hpp>

template <CSerializable ...Keys>
using MultiLevelKey = std::tuple<Keys...>;

template <CSerializable Value, CSerializable ...Keys>
    requires std::is_default_constructible_v<Value>
class SimpleMultiLevelKeyMap {
    std::map<MultiLevelKey<Keys...>, Value> m;

public:
    SimpleMultiLevelKeyMap() {}
    ~SimpleMultiLevelKeyMap() {}

    Result add(const Keys&... keys, const Value &value){
        m.emplace(std::make_tuple(keys...), value);
        return Result::Success;
    }
    std::pair<bool, Value> get(const Keys&... keys){
        auto it = m.find(std::make_tuple(keys...));
        if(it == m.end()){
            return std::make_pair(false, Value{});
        }
        return std::make_pair(true, it->second);
    }
    Value &getRef(const Keys&... keys){
        auto it = m.find(std::make_tuple(keys...));
        if(it == m.end()){
            return std::make_pair(false, Value{});
        }
        return std::make_pair(true, it->second);
    }
    void del(const Keys&... keys){
        m.erase(std::make_tuple(keys...));
    }
    //CSerializableImpl
    Result serializeImpl(StorageBuffer<> &buffer) const {
		size_t offset = 0;
        auto m_size = m.size();
		auto buf = buffer.offset_advance(offset, szeimpl::size(m_size));
		szeimpl::s(m_size, buf);
		for(auto it: m){
			buf = buffer.offset_advance(offset, szeimpl::size(it));
			szeimpl::s(it, buf);
		}
        return Result::Success;
    }
    static SimpleMultiLevelKeyMap<Value, Keys...>
    deserializeImpl(const StorageBufferRO<> &buffer) {
        SimpleMultiLevelKeyMap<Value, Keys...> smlmk;

		size_t offset = 0;
		auto buf = buffer;
		auto num = szeimpl::d<typename decltype(smlmk.m)::size_type>(buf);
		buf = buffer.advance_offset(offset, szeimpl::size(num));
		for(size_t i = 0; i < num; i++){
			auto val = szeimpl::d<typename decltype(smlmk.m)::value_type>(buf);
            if(i != num - 1)
			    buf = buffer.advance_offset(offset, szeimpl::size(val));
			smlmk.m.emplace(val);
		}
        return smlmk;
    }
    size_t getSizeImpl() const {
        size_t sum = 0;
        sum += szeimpl::size(m.size());
        for(auto &e: m){
            sum += szeimpl::size(e);
        }
        return sum;
    }
};


/*
VFC - per DataStorage instance - to organize all entries
    Value - StorageAddress, ObjectAddress
    Key - any
VFC - Group of Storage instances - by any type
    Value - DataStorage
    Key - any
    Scope - most likely global

Two options: we only store VFC in DataStorage, in static section
             or we provide StorageAddress pointing to VFC metadata, which points to actual data for VFC

Required Feature - verify VFC type stored at the address
*/
template <CStorageImpl Storage, CSerializable Value, CSerializable ...Keys>
    requires std::is_default_constructible_v<Value>
class VirtualFileCatalog{
    Storage &storage;
    StorageAddress static_header_address;
    StorageBuffer<> static_header_buffer;
    StaticHeader *static_header;

    using MLKM = SimpleMultiLevelKeyMap<Value, Keys...>;
    MLKM mlkm;

    static constexpr uint64_t VFC_MAGIC = 0x5544F79A;
    static constexpr size_t DEFAULT_ALLOC_SIZE = (1 << 10); //1KB

    void init_vfc(){
        ASSERT_ON(static_header_address.size < szeimpl::size(StaticHeader{}));
        if(static_header->magic != VFC_MAGIC){
            //new
            static_header->magic = VFC_MAGIC;
            static_header->address = storage.template get_random_address(DEFAULT_ALLOC_SIZE);
        } else {
            //deserialize mlkm
            mlkm = deserialize<MLKM>(storage, static_header->address);
        }
    }
public:
    // addr is at least szeimpl::size(StaticHeader{})
    VirtualFileCatalog(Storage &storage, StorageAddress &addr): //TODO write test: simple,  VFC<VFC>
        storage(storage),
        static_header_address(addr),
        static_header_buffer(storage.template writeb(static_header_address)),
        static_header(static_header_buffer.get<StaticHeader>()) {
            init_vfc();
    }
    VirtualFileCatalog(Storage &storage):
        storage(storage),
        static_header_address(storage.template get_static_section()),
        static_header_buffer(storage.template writeb(static_header_address)),
        static_header(static_header_buffer.get<StaticHeader>()) {
            init_vfc();
    }
    ~VirtualFileCatalog(){
        serialize(storage, mlkm, static_header->address);
        storage.template commit(static_header_buffer);
    }

    Result add(const Keys&... keys, const Value &value){
        return mlkm.add(keys..., value);
    }
    std::pair<bool, Value> get(const Keys&... keys){
        return mlkm.get(keys...);
    }
    std::pair<bool, Value &> getRef(const Keys&... keys){
        return mlkm.getRef(keys...);
    }
    void del(const Keys&... keys){
        mlkm.del(keys...);
    }

    Result serializeImpl(StorageBuffer<> &buffer) const {
        Result res = Result::Success;
        size_t offset = 0;
        StorageBuffer buf;
        std::string type_name{typeid(decltype(*this)).name()};
    
        buf = buffer.offset_advance(offset, szeimpl::size(type_name));
        res = szeimpl::s(type_name, buf);
    
        buf = buffer.offset_advance(offset, szeimpl::size(mlkm));
        res = szeimpl::s(mlkm, buf);
        return res;
    }
    //pointer to other storage - on which VFC opearates on
    static VirtualFileCatalog<Storage, Value, Keys...>
        deserializeImpl(const StorageBufferRO<> &buffer, Storage &storage) {
            //TODO not implemented. should not be called. use construction
            ASSERT_ON_MSG(false, "Not implemented. Use constructor instead");
            size_t offset = 0;
            auto buf = buffer;
            auto type_name = szeimpl::d<std::string>(buf);
            buf = buffer.advance_offset(offset, szeimpl::size(type_name));
            ASSERT_ON(type_name != std::string{typeid(VirtualFileCatalog<Storage, Value, Keys...>).name()});
            //verify type name
            return VirtualFileCatalog<Value, Keys...>{storage};
    }
    size_t getSizeImpl() const {
        std::string type_name{typeid(decltype(*this)).name()};
        return szeimpl::size(mlkm) + szeimpl::size(type_name);
    }
    /*
    static VirtualFileCatalog Create(DataStorage &storage){ //should it be a singleton?
        VirtualFileCatalog vfc{storage};
    }*/
};

/*
(De)Serializing
What is dependency ObjectStorage from SimpleMultiLevelKeyMap<>? None
ObjectStorage ref can be Value

*/
