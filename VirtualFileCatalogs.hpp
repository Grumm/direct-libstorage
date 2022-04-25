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