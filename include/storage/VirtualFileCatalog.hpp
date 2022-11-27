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
        SerializeSequentially(buffer, offset, m.size());

		for(auto it: m){
            SerializeSequentially(buffer, offset, it);
		}
        return Result::Success;
    }
    static SimpleMultiLevelKeyMap<Value, Keys...>
    deserializeImpl(const StorageBufferRO<> &buffer) {
        SimpleMultiLevelKeyMap<Value, Keys...> smlmk;
		size_t offset = 0;
		auto buf = buffer;

		auto [num] = DeserializeSequentially<typename decltype(smlmk.m)::size_type>(buf, offset);
		for(size_t i = 0; i < num; i++){
			auto [val] = DeserializeSequentially<typename decltype(smlmk.m)::value_type>(buf, offset);
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
template <CSerializable Value, CSerializable ...Keys>
    requires std::is_default_constructible_v<Value>
class VirtualFileCatalogImpl{
    using MLKM = SimpleMultiLevelKeyMap<Value, Keys...>;
    MLKM mlkm;
public:
    VirtualFileCatalogImpl(MLKM &&mlkm):
        mlkm(mlkm) {}
    VirtualFileCatalogImpl() = default;
    ~VirtualFileCatalogImpl() = default;

    Result add(const Keys&... keys, const Value &value){
        return mlkm.add(keys..., value);
    }
    bool has (const Keys&... keys){
        //TODO
        return false;
    }
    std::pair<bool, Value> get(const Keys&... keys){
        return mlkm.get(keys...);
    }/*
    template<typename T>
    std::pair<bool, Value> get(const Keys&... keys){
        return static_cast<T>(mlkm.get(keys...));
    }*/
    std::pair<bool, Value &> getRef(const Keys&... keys){
        return mlkm.getRef(keys...);
    }
    void del(const Keys&... keys){
        mlkm.del(keys...);
    }

    Result serializeImpl(StorageBuffer<> &buffer) const {
        Result res = Result::Success;
        size_t offset = 0;

        return SerializeSequentially(buffer, offset, mlkm);
    }
    //pointer to other storage - on which VFC opearates on
    template<CStorage Storage>
    static VirtualFileCatalogImpl
        deserializeImpl(const StorageBufferRO<> &buffer, Storage &storage) {
            size_t offset = 0;
            auto buf = buffer;

            auto [mlkm] = DeserializeSequentially<MLKM>(buf, offset);

            return VirtualFileCatalogImpl{std::move(mlkm)};
    }
    static VirtualFileCatalogImpl deserializeImpl(const StorageBufferRO<> &buffer) { throw std::bad_function_call(); }
    size_t getSizeImpl() const {
        std::string type_name{typeid(decltype(*this)).name()};
        return szeimpl::size(mlkm) + szeimpl::size(type_name);
    }
};

template <CSerializable Value, CSerializable ...Keys>
using TypedVirtualFileCatalog = TypedObject<VirtualFileCatalogImpl<Value, Keys...>>;

template <CStorage Storage, CSerializable Value, CSerializable ...Keys>
using VirtualFileCatalog = AutoStoredObject<TypedVirtualFileCatalog<Value, Keys...>, Storage>;

/*
(De)Serializing
What is dependency ObjectStorage from SimpleMultiLevelKeyMap<>? None
ObjectStorage ref can be Value

*/
