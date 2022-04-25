#pragma once

#include <DataStorage.hpp>


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

class AbstractVirtualFileEntity{

};

template <typename ...Keys>
using VFCPath = std::tuple<Keys...>;

template <typename ...Keys>
class VirtualFileCatalog{
    StorageAddress base;
    DataStorage &storage;
    std::map<VFCPath<Keys...>, StorageAddress> m;
    void deserialize(){
        m.clear();

    }
    void serialize(){
        //put m into 
    }
public:
    VirtualFileCatalog(DataStorage &storage): storage(storage){
        //init base
        deserialize();
    }

    Result add(const Keys... &keys, const StorageAddress &addr){
        m.emplace(std::make_tuple(keys...), addr);
        return Result::Success;
    }
    StorageAddress get(const Keys... &keys){
        auto it = m.find(std::make_tuple(keys...));
        ASSERT_ON(it == m.end());
        return it->second;
    }
    ~VirtualFileCatalog(){
        serialize();
    }
};

//TODO StaticObject which maps string to StorageAddress and VFC path

//usage
VirtualFileCatalog

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

template <typename Value, typename Key, typename ...Keys>
class MultiLevelKeyMap {
//single-layer
    std::map<std::tuple<Key, Keys...>, Value> m;
//recursive:
//simple
    std::map<Key, MultiLevelKeyMap<Value, Keys...>> m;
//optimal?
    std::vector<MultiLevelKeyMap<Value, Keys...>> values;
    std::map<Key, size_t> m; //maps to values index 

public:
    MultiLevelKeyMap() {}

    std::set<Key> list(){}

    Result add(const Key &key, const Keys... &keys, Value &value){
        auto it = m.find(key);
        if(it == m.end()){
            it = m.emplace(key, MultiLevelKeyMap<Value, Keys...>{});
        }
        return it->second.add(keys..., value);
    }
    std::pair<bool, Value> get(const Key &key, const Keys... &keys){
        auto it = m.find(key);
        if(it == m.end()){
            return std::make_pair(false, Value{});
        }
        return it->second.get(keys...);
    }
};


template <typename Value, typename Key>
class MultiLevelKeyMap {
//simple
    std::map<Key, Value> m;
//optimal?
    std::vector<Value> values;
    std::map<Key, size_t> m;
}

//or just simple one


#include <Serialize.hpp>

template <CSerializable ...Keys>
using MultiLevelKey = std::tuple<Keys...>;

template <CSerializable Value, CSerializable ...Keys>
    requires DefaultConstructible<Value>
class SimpleMultiLevelKeyMap:
        public ISerialize<SimpleMultiLevelKeyMap<Value, Keys...>> {
    std::map<MultiLevelKey<Keys...>, Value> m;

public:
    SimpleMultiLevelKeyMap() {}
    ~SimpleMultiLevelKeyMap() {}

    Result add(const Keys... &keys, const Value &value){
        m.emplace(std::make_tuple(keys...), value);
        return Result::Success;
    }
    std::pair<bool, Value> get(const Keys... &keys){
        auto it = m.find(std::make_tuple(keys...));
        if(it == m.end()){
            return std::make_pair(false, Value{});
        }
        return std::make_pair(true, it->second);
    }
    std::pair<bool, Value &> getRef(const Keys... &keys){
        auto it = m.find(std::make_tuple(keys...));
        if(it == m.end()){
            return std::make_pair(false, Value{});
        }
        return std::make_pair(true, it->second);
    }
    //TODO delete()
    //ISerialize
    Result serializeImpl(const StorageBuffer &buffer) const { throw std::bad_function_call("Not implemented"); }
    template <CSerializable Value, CSerializable ...Keys>
    static SimpleMultiLevelKeyMap<Value, Keys...>
    deserializeImpl(const StorageBufferRO &buffer) { throw std::bad_function_call("Not implemented"); }
    size_t getSizeImpl() const { throw std::bad_function_call("Not implemented"); }
};

template <CSerializable Value, CSerializable ...Keys>
    requires DefaultConstructible<Value>
class VirtualFileCatalog: public ISerialize<Keys...>{
    DataStorage &storage;
    StorageAddress static_header_address;
    StorageBuffer static_header_buffer;
    StaticHeader *static_header;

    using MLKM = SimpleMultiLevelKeyMap<Value, Keys...>;
    MLKM mlkm;

    static constexpr size_t VFC_MAGIC = 0x5544F79A;
    static constexpr size_t DEFAULT_ALLOC_SIZE = 1024;
public:
    VirtualFileCatalog(DataStorage &storage):
        storage(storage),
        static_header_address(storage.get_static_section()),
        static_header_buffer(storage.writeb(static_header_address)),
        static_header(static_cast<StaticHeader *>(static_header_buffer.data)) {
        if(static_header->magic != VFC_MAGIC){
            //new
            static_header->magic = VFC_MAGIC;
            static_header->address = storage.get_random_address(DEFAULT_ALLOC_SIZE);
        } else {
            //deserialize mlkm
            mlkm = deserialize<MLKM>(storage, static_header->address);
        }
    }
    ~VirtualFileCatalog(){
        serialize(storage, mlkm, static_header->address);
        storage.commit(static_header->address);
        storage.commit(static_header_address);
    }

    //get
    //add
    //delete
};

/*
(De)Serializing
What is dependency ObjectStorage from SimpleMultiLevelKeyMap<>? None
ObjectStorage ref can be Value

*/