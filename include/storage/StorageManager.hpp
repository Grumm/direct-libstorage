#pragma once
/*
//tuple
template < typename T, typename U >
struct typelist {
    typedef T element_type;
    typedef U next_type;
};
template < class T, int INDEX >
struct get_type {
    typedef typename get_type < typename T::next_type,
                                INDEX-1 >::type type;
};
template < class T >
struct get_type < T, 0 > {
    typedef typename T::element_type type;
};



class StorageManager{
public:
    using StorageIdType = uint32_t;

	  template<typename T, typename K>
	  ObjectStorage<T,K> &getObjectStorageInstance(StorageIdType id);

	  DataStorage &getDataStorageInstance(StorageIdType id);
};
*/



#include <storage/StorageUtils.hpp>
#include <storage/UniqueID.hpp>
#include <storage/Serialize.hpp>
#include <storage/RandomMemoryAccess.hpp>
#include <storage/SimpleStorage.hpp>

template<CStorage Storage = SimpleFileStorage<24>>
class StorageManager {
    Storage &storage;
    struct Metadata{
        static constexpr uint32_t MAGIC = 0xde0a2Fb;
        uint32_t magic;
        StorageAddress uid;
        //TODO like type table for all uids?
        void init(Storage &storage){
            if(magic != MAGIC){
                uid = storage.get_random_address(szeimpl::size(UniqueIDStorage<DataStorageBase, Storage>{storage}));
                initialize_zero(storage, uid);
            }
        }

        static constexpr size_t size(){
            return sizeof(Metadata);
        }
    };
    StorageAddress metadata_addr;
    Metadata metadata;

    //to initialized via init() - second stage, needs g_storage_manager initialized already
    std::unique_ptr<UniqueIDStorage<DataStorageBase, Storage>> uid;
public:
    static constexpr size_t METADATA_SIZE = Metadata::size();
    StorageManager(Storage &storage):
            storage(storage),
            metadata_addr(storage.template get_static_section()),
            metadata(deserialize<Metadata>(storage, metadata_addr)) {
        ASSERT_ON(METADATA_SIZE > metadata_addr.size);
        metadata.init(storage);
    }
    void init(){
        auto ptr = deserialize_ptr<UniqueIDStorage<DataStorageBase, Storage>>(storage, metadata.uid, storage);
        uid.swap(ptr);
    }
    Storage &getStorage() { return storage; }
    UniqueIDStorage<DataStorageBase, Storage> &getUIDStorage() { return *uid.get(); }
};

/************************************************************************/

//Factory to create DataStorage instance from id

//We don't keep DataStorage full description in VirtFileCatalog, only it's id(and type?)
//We have to be able to create Instance from id and type -> Factory to create an instance from id and type
//We need to have type as a factory to create, id is mapped onto DataStorage::serializeImpl result or an instance
//Factory utilizes deserializeImpl to create instance
//We also need to be able to create instance for StorageManager, so DataStorage implementation has to have no link to StorageManager, but DataStorage might(?)
//Factory to fetch the local storage of already instantiated -> therefore DataStorage is a singletone with a keys(id, type)

