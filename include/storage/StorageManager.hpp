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

#include <type_traits>
#include <tuple>
#include <cstdint>
#include <map>
#include <memory>
#include <filesystem>


#include <StorageUtils.hpp>
#include <Serialize.hpp>
#include <RandomMemoryAccess.hpp>
#include <SimpleStorage.hpp>

/************************************************************************/

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

//DataStorage: public UniqueIDInstance {}

/************************************************************************/

template<typename T>
class UniqueIDStorage {
    DataStorage &storage;
    std::map<uint32_t, std::pair<StorageAddress, std::unique_ptr<T>>> m;
    uint32_t max_id{1};
public:
    UniqueIDStorage(DataStorage &storage): storage(storage) {}

    template<typename U = T>
    void RegisterUniqueInstance(const U &u){
        auto t_ptr = dynamic_cast<T *>(&u);
        ASSERT_ON(t_ptr == nullptr);
        auto id = t.UniqueIDInstance::getUniqueID();
        auto it = m.find(id);
        if(it != m.end()){
            ASSERT_ON(it->second.second.get() != nullptr);
            it->second.second = std::make_unique<T>(t_ptr);
        } else {
            m.emplace(id, StorageAddress{}, t_ptr);
        }
    }
    template<typename U> //requires derived 
    U &GetUniqueInstance(const UniqueIDInstance &instance) const{
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
    //TODO serialize impl
    Result serializeImpl(StorageBuffer<> &buffer) const {
		ASSERT_ON(getSizeImpl() > buffer.allocated());
		size_t offset = 0;

		auto buf = buffer.offset_advance(offset, szeimpl::size(max_id));
		szeimpl::s(max_id, buf);

		auto m_size = m.size();
		buf = buffer.offset_advance(offset, szeimpl::size(m_size));
		szeimpl::s(m_size, buf);

		for(auto it: m){
			buf = buffer.offset_advance(offset, szeimpl::size(it));
			szeimpl::s(it, buf);
		}
    }
    static T deserializeImpl(const StorageBufferRO<> &buffer) {
        UniqueIDStorage<T> uid{GetStorageManager().getStorage()};

		size_t offset = 0;
		auto buf = buffer;
		uid.max_id = szeimpl::d<decltype(uid.max_id)>(buf);
		buf = buffer.advance_offset(offset, szeimpl::size(uid.max_id));

		auto num = szeimpl::d<decltype(m)::size_type>(buf);
		buf = buffer.advance_offset(offset, szeimpl::size(num));
		for(size_t i = 0; i < num; i++){
			auto val = szeimpl::d<decltype(m)::value_type>(buf);
			buf = buffer.advance_offset(offset, szeimpl::size(val));
			vam.m.emplace(val);
		}
    }
    size_t getSizeImpl() const {
        size_t sum = szeimpl::size(max_id) + szeimpl::size(decltype(m)::size_type{0}));
        for(auto &e: m){
            sum += szeimpl::size(e);
        }
        return sum;
    }
};

/************************************************************************/

class StorageManager {
    static constexpr uint32_t FILESIZE = 24; //16MB
    FileRMA<FILESIZE> rma;
    SimpleStorage storage;

    struct Metadata{
        static constexpr uint32_t MAGIC = 0xde0a2Fb;
        uint32_t magic;
        StorageAddress uid;
        //TODO like type table for all uids?
        void init(DataStorage &storage){
            if(magic != MAGIC){
                uid = storage.get_random_address(szeimpl::size(UniqueIDStorage<DataStorage>{storage}));
            }
        }
    };
    StorageAddress metadata_addr;
    Metadata metadata;

    //to initialized via init() - second stage, needs g_storage_manager initialized already
    std::unique_ptr<UniqueIDStorage<DataStorage>> uid;
public:
    StorageManager(const std::string &filename):
            rma(filename),
            storage(rma),
            metadata_addr(storage.get_static_section()),
            metadata(deserialize<Metadata>(storage, metadata_addr)) {
        metadata.init(storage);
    }
    void init(){
        uid.swap(deserialize_ptr<UniqueIDStorage<DataStorage>>(storage, metadata.uid));
    }
    SimpleStorage &getStorage() { return storage; }
    UniqueIDStorage<DataStorage> &getUIDStorage() { return *uid.get(); }
};

/************************************************************************/

class StorageManager;
extern std::unique_ptr<StorageManager> g_storage_manager;

//generate filename based on argv[0]
static std::string generate_filename_from_argv(std::string execname){
    std::filesystem::path path{execname};
    path = std::filesystem::absolute(path);
    execname = path.string();
    std::replace(execname.begin(), execname.end(), '/', '_');
    std::replace(execname.begin(), execname.end(), '\\', '_');
    std::replace(execname.begin(), execname.end(), ':', '_');
    std::replace(execname.begin(), execname.end(), '.', '_');
    return execname;
}

void InitLibStorage(int argc, const char *argv[]){
    static const std::string DEFAULT_PATH{"~/.libstorage/"s};
    std::string filename = generate_filename_from_argv(std::string{argv[0]});
    std::filesystem::path path{DEFAULT_PATH};
    path.replace_filename(filename);
    path = std::filesystem::absolute(path);

    ASSERT_ON(!std::filesystem::create_directories(path));
    g_storage_manager.reset(new StorageManager(path.string()));
    g_storage_manager->init();
}

void InitLibStorage(const std::string &filename){
    std::filesystem::path path{filename};
    path = std::filesystem::absolute(path);

    ASSERT_ON(!std::filesystem::create_directories(path));
    g_storage_manager.reset(new StorageManager(path.string()));
    g_storage_manager->init();
}

StorageManager &GetStorageManager(){
    ASSERT_ON(g_storage_manager.get() == nullptr, "Uninitialzied Storage");
    return *g_storage_manager.get();
}

//Factory to create DataStorage instance from id

//We don't keep DataStorage full description in VirtFileCatalog, only it's id(and type?)
//We have to be able to create Instance from id and type -> Factory to create an instance from id and type
//We need to have type as a factory to create, id is mapped onto DataStorage::serializeImpl result or an instance
//Factory utilizes deserializeImpl to create instance
//We also need to be able to create instance for StorageManager, so DataStorage implementation has to have no link to StorageManager, but DataStorage might(?)
//Factory to fetch the local storage of already instantiated -> therefore DataStorage is a singletone with a keys(id, type)

