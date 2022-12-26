#include <memory>
#include <string>
#include <filesystem>

#include <storage/Storage.hpp>

constexpr size_t GStorageSize = 24;
using GStorageType = SimpleFileStorage<GStorageSize>;
std::unique_ptr<GStorageType> g_storage_base;
std::unique_ptr<StorageManager<>> g_storage_manager;


//generate filename based on argv[0]
std::string generate_filename_from_argv(std::string execname){
    std::filesystem::path path{execname};
    path = std::filesystem::absolute(path);
    execname = path.string();
    std::replace(execname.begin(), execname.end(), '/', '_');
    std::replace(execname.begin(), execname.end(), '\\', '_');
    std::replace(execname.begin(), execname.end(), ':', '_');
    std::replace(execname.begin(), execname.end(), '.', '_');
    return execname;
}

static void init_libstorage(std::filesystem::path path){
    ASSERT_ON(!path.has_filename());
    auto filename = path.filename();
    path.remove_filename();
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    ASSERT_ON_MSG(ec, ec.message());
    path.replace_filename(filename);
    g_storage_base.reset(new GStorageType(FileRMA<GStorageSize>{filename}, StorageManager<GStorageType>::METADATA_SIZE));
    g_storage_manager.reset(new StorageManager<GStorageType>(*g_storage_base.get()));
    g_storage_manager->init();
}

void InitLibStorage([[maybe_unused]] int argc, const char *argv[]){
    static const std::string DEFAULT_PATH{"~/.libstorage/"};
    std::string filename = generate_filename_from_argv(std::string{argv[0]});
    std::filesystem::path path{DEFAULT_PATH};
    path.replace_filename(filename);
    path = std::filesystem::absolute(path);

    init_libstorage(path);
}

void InitLibStorage(const std::string &_path, const std::string &filename){
    std::filesystem::path path{_path};
    path = std::filesystem::absolute(path);
    path.replace_filename(filename);

    init_libstorage(path);
}

bool HaveStorageManager(){
    return g_storage_manager.get() != nullptr;
}
StorageManager<GStorageType> &GetStorageManager(){
    ASSERT_ON_MSG(!HaveStorageManager(), "Uninitialzied Storage");
    return *g_storage_manager.get();
}

DataStorageBase &GetGlobalMetadataStorage(){
    return GetStorageManager().getStorage();
}

UniqueIDStorage<DataStorageBase, GStorageType> &GetGlobalUniqueIDStorage(){
    return GetStorageManager().getUIDStorage();
}

uint32_t GenerateGlobalUniqueID(){
    if(HaveStorageManager()){
        return GetGlobalUniqueIDStorage().generateID();
    } else {
        return UniqueIDStorage<DataStorageBase, GStorageType>::UNKNOWN_ID; //Global Metadata
    }
}
