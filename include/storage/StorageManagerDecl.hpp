#pragma once

#include <string>
class StorageManager;
class DataStorage;
template <typename T>
class UniqueIDStorage;

extern StorageManager &GetStorageManager();
extern DataStorage &GetGlobalMetadataStorage();
extern UniqueIDStorage<DataStorage> &GetGlobalUniqueIDStorage();
extern std::string generate_filename_from_argv(std::string execname);
extern bool HaveStorageManager();
extern uint32_t GenerateGlobalUniqueID();

//UniqueIDStorage<DataStorage> &GetUIDStorage();
extern void InitLibStorage(const std::string &_path, const std::string &filename);
extern void InitLibStorage(int argc, const char *argv[]);

