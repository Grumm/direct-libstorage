#pragma once

#include <type_traits>
#include <functional>
#include <memory>

#include <storage/Utils.hpp>
#include <storage/StorageUtils.hpp>
#include <storage/SerializeImpl.hpp>
#include <storage/SerializeSpecialization.hpp>
#include <storage/UniqueIDInterface.hpp>

class DataStorageBase;

template<typename T>
concept CStorageGeneric = requires(T &storage, const StorageAddress &addr, size_t size){
    { storage.get_static_section() } -> std::same_as<StorageAddress>;
    { storage.get_random_address(size) } -> std::same_as<StorageAddress>;
    { storage.expand_address(addr, size) } -> std::same_as<StorageAddress>;
    { storage.erase(addr) } -> std::same_as<Result>;
};

template<typename T>
concept CStorageDirectMappedIO = requires(T &storage, const StorageAddress &addr, const StorageBuffer<> &const_buffer,
        const StorageBufferRO<> &const_buffer_ro){
    { storage.writeb(addr) } -> std::same_as<StorageBuffer<>>;
    { storage.readb(addr) } -> std::same_as<StorageBufferRO<>>;
    { storage.commit(const_buffer) } -> std::same_as<Result>;
    { storage.commit(const_buffer_ro) } -> std::same_as<Result>;
};

template<typename T>
concept CStorageCopybackIO = requires(T &storage, const StorageAddress &addr, StorageBuffer<> &buffer, const StorageBuffer<> &const_buffer){
    { storage.read(addr, const_buffer) } -> std::same_as<Result>;
    { storage.write(addr, const_buffer) } -> std::same_as<Result>;
    { storage.write(addr) } -> std::same_as<Result>;
    { storage.write(addr, buffer) } -> std::same_as<Result>;
};

template<typename T, typename U = void>
concept CStorageNoRef = std::is_base_of_v<DataStorageBase, T> && CSerializable<T> && CStorageGeneric<T> && CStorageDirectMappedIO<T>;
template<typename T, typename U = void>
concept CStorage = CStorageNoRef<std::remove_cv_t<std::remove_reference_t<T>>, U>;



/*
class DSInterfaceCast{
public:
    template<typename T>
        requires std::negation_v<std::is_same<T, void>>
    Result read(const StorageAddress &addr, StorageBuffer<T> &buffer){
        return read(addr, buffer.template cast<void>());
    }
    template<typename T>
        requires std::negation_v<std::is_same<T, void>>
    Result write(const StorageAddress &addr, const StorageBuffer<T> &buffer){
        return write(addr, buffer.template cast<void>());
    }
    template<typename T>
        requires std::negation_v<std::is_same<T, void>>
    Result commit(const StorageBuffer<T> &buffer){
        return commit(buffer.template cast<void>());
    }
    template<typename T>
        requires std::negation_v<std::is_same<T, void>>
    Result commit(const StorageBufferRO<T> &buffer){
        return commit(buffer.template cast<void>());
    }
    template<typename T>
        requires std::negation_v<std::is_same<T, void>>
    StorageBuffer<T> writeb(const StorageAddress &addr){
        return writeb(addr).template cast<T>();
    }
    template<typename T>
        requires std::negation_v<std::is_same<T, void>>
    StorageBufferRO<T> readb(const StorageAddress &addr){
        return writeb(addr).template cast<T>();
    }
};*/

class DataStorageBase: public UniqueIDInstance {
public:
    template<typename T>
        requires std::negation_v<std::is_same<T, void>>
    Result read(const StorageAddress &addr, StorageBuffer<T> &buffer){
        return read(addr, buffer.template cast<void>());
    }

    DataStorageBase(uint32_t id): UniqueIDInstance(id) {
        /*	DataStorage(): UniqueIDInstance(GenerateGlobalUniqueID()) {}
        if(HaveStorageManager()){
            GetGlobalUniqueIDStorage().registerInstance<DataStorage>(*this);
        } */
    }
    virtual ~DataStorageBase(){}
};

//TODO reorganize
template<CStorage Storage>
static inline Result initialize_zero(Storage &storage, const StorageAddress &addr){
    auto buf = storage.writeb(addr);
    memset(buf.get(), 0, buf.allocated());
    return storage.commit(buf);
}

#if 0
template <std::size_t...Idxs>
constexpr auto substring_as_array(std::string_view str, std::index_sequence<Idxs...>)
{
  return std::array{str[Idxs]..., '\n'};
}

template <typename T>
constexpr auto type_name_array()
{
#if defined(__clang__)
  constexpr auto prefix   = std::string_view{"[T = "};
  constexpr auto suffix   = std::string_view{"]"};
  constexpr auto function = std::string_view{std::source_location{}.function_name()};
#elif defined(__GNUC__)
  constexpr auto prefix   = std::string_view{"with T = "};
  constexpr auto suffix   = std::string_view{"]"};
  constexpr auto function = std::string_view{std::source_location{}.function_name()};
#elif defined(_MSC_VER)
  constexpr auto prefix   = std::string_view{"type_name_array<"};
  constexpr auto suffix   = std::string_view{">(void)"};
  constexpr auto function = std::string_view{std::source_location{}.function_name()};
#else
# error Unsupported compiler
#endif

  constexpr auto start = function.find(prefix) + prefix.size();
  constexpr auto end = function.rfind(suffix);

  static_assert(start < end);

  constexpr auto name = function.substr(start, (end - start));
  return substring_as_array(name, std::make_index_sequence<name.size()>{});
}

template <typename T>
struct type_name_holder {
  static inline constexpr auto value = type_name_array<T>();
};

template <typename T>
constexpr auto type_name() -> std::string_view
{
  constexpr auto& value = type_name_holder<T>::value;
  return std::string_view{value.data(), value.size()};
}
#endif

//template<typename T = void>
struct StaticHeader{
    uint64_t magic;
    StorageAddress address;
    //TODO serialize deserialize, add type_name<T> check
};

template<typename T>
constexpr uint32_t make_magic(){
    //TODO use typename and constexpr hash function
    return 0xDEEFAB;
}

template<typename T, uint32_t STORAGE_MAGIC = make_magic<T>()>
class TypedObject{
    T object;
public:
    //we assume address already contains the object

    TypedObject(T &&t):
            object(std::forward<T>(t)) { }

    TypedObject(uint32_t magic, T &&t):
            object(std::forward<T>(t)) {
        if (verify(magic)){
            throw std::invalid_argument("Incorrect magic " + std::to_string(magic) + " != " +
                                        std::to_string(STORAGE_MAGIC) + " for expected type " + typeid(T).name());
        }
    }

    template<typename ...Args>
    TypedObject(Args& ...args):
            object(std::forward<Args &>(args)...) { }

    T &get(){ return object; }
    const T &get() const { return object; }

    static constexpr bool verify(uint32_t magic) {
        return STORAGE_MAGIC == magic;
    }

    Result serializeImpl(StorageBuffer<> &buffer) const {
        Result res = Result::Success;
        size_t offset = 0;
        return SerializeSequentially(buffer, offset, STORAGE_MAGIC, object);
    }
    template<typename ...Args, typename ...Args2>
    static TypedObject deserializeImpl(const StorageBufferRO<> &buffer, Args2& ...args2, Args& ...args) {
        size_t offset = 0;
        auto buf = buffer;

        auto [magic] = DeserializeSequentially<uint32_t>(buf, offset);
        if(!verify(magic)){
            return TypedObject{std::forward<Args2 &>(args2)...};
        }

        auto [object] = DeserializeSequentially<T>(buf, offset, std::forward<Args &>(args)...);

        return TypedObject{object};
    }
    static TypedObject deserializeImpl(const StorageBufferRO<> &) { throw std::bad_function_call(); };
    constexpr size_t getSizeImpl() const {
        return szeimpl::size(uint32_t{}) + szeimpl::size(object);
    }
};

template<CSerializable T, size_t DEFAULT_ALLOC_SIZE = (1UL << 20)>
class StoredObject{
protected:
    StorageAddress address;
    bool modified;
    T object;
public:
    //assuming address already contains an object
    template<CStorage Storage, typename ...Args>
    StoredObject(const StorageAddress &address, Storage &storage, Args... args):
        address(address),
        modified{false},
        object(deserialize<T>(storage, address, std::forward<Args>(args)...)) {}

    //newly created object
    template<CStorage Storage>
    StoredObject(T &&t, Storage &storage):
        address(storage.template get_random_address(DEFAULT_ALLOC_SIZE)),
        modified{true},
        object(std::forward<T>(t)) {}

    //newly created object in-place
    template<CStorage Storage, typename ...Args>
    StoredObject(Storage &storage, Args& ...args):
        address(storage.template get_random_address(DEFAULT_ALLOC_SIZE)),
        modified{true},
        object(std::forward<Args &>(args)...) {}

    T &get(){ modified = true; return object; }
    const T &get() const { return object; }

    template<CStorage Storage>
    void sync(Storage &storage){
        if(modified){
            modified = false;
            serialize(storage, object, address);
        }
    }
    //TODO add copy, move, compare, hash

    Result serializeImpl(StorageBuffer<> &buffer) const {
        Result res = Result::Success;
        size_t offset = 0;
        return SerializeSequentially(buffer, offset, address);
    }
    template<CStorage Storage, typename ...Args>
    static StoredObject deserializeImpl(const StorageBufferRO<> &buffer, Storage &storage, Args&& ...args) {
        size_t offset = 0;
        auto buf = buffer;

        auto [address] = DeserializeSequentially<StorageAddress>(buf, offset);

        return StoredObject{address, storage, std::forward<Args>(args)...};
    }
    static StoredObject deserializeImpl(const StorageBufferRO<> &) { throw std::bad_function_call(); };
    constexpr size_t getSizeImpl() {
        return szeimpl::size(StorageAddress{});
    }
};

template<CSerializable T, CStorage Storage, size_t DEFAULT_ALLOC_SIZE = (1UL << 20)>
class AutoStoredObject: public StoredObject<T, DEFAULT_ALLOC_SIZE>{
    Storage &storage;
public:
    //assuming address already contains an object
    template<typename ...Args>
    AutoStoredObject(const StorageAddress &address, Storage &storage, Args&& ...args):
        StoredObject<T, DEFAULT_ALLOC_SIZE>(address, storage, std::forward<Args>(args)...),
        storage(storage) {}

    //newly created object
    AutoStoredObject(T &&t, Storage &storage):
        StoredObject<T, DEFAULT_ALLOC_SIZE>(std::forward<T>(t), storage),
        storage(storage) {}

    //newly created object in-place
    template<typename ...Args>
    AutoStoredObject(Storage &storage, Args& ...args):
        StoredObject<T, DEFAULT_ALLOC_SIZE>(storage, std::forward<Args &>(args)...),
        storage(storage) {}

    ~AutoStoredObject(){
        this->sync(storage);
    }
    //TODO add copy, move, compare, hash

    Result serializeImpl(StorageBuffer<> &buffer) const {
        Result res = Result::Success;
        size_t offset = 0;
        return SerializeSequentially(buffer, offset, this->object); //storage.UniqueIDInstance::getUniqueID()
        //TODO should we serialize storage.UniqueIDInstance::getUniqueID() here as well?
    }
    template<typename ...Args>
    static AutoStoredObject deserializeImpl(const StorageBufferRO<> &buffer, Storage &storage, Args& ...args) {
        size_t offset = 0;
        auto buf = buffer;

        auto [object] = DeserializeSequentially<T>(buf, offset, std::forward<Args &>(args)...);

        return AutoStoredObject{std::move(object), storage};
    }

    static AutoStoredObject deserializeImpl(const StorageBufferRO<> &buffer) { throw std::bad_function_call(); };
    constexpr size_t getSizeImpl() const {
        return szeimpl::size(StoredObject<T, DEFAULT_ALLOC_SIZE>::object);
    }
};

/*
For DataStorage we need some way to store some static information.
Q1: way to identify static address:
- defined in DataStorage constant, fixed address
- queried from DataStorage object

Q2: how to allocate
- allocated on storage creation automatically
- allocated on demand

Q3: what we store there:
- whole section, which maps multiple: string key -> storage address
- one storage address
- unspecified

A: Q1 - don't want to overcomplicate DataStorage interface, fixed const address
A: Q2 - to simplify storage object creation, make it a interface requirement?
A: Q3 - map would be nice, but 

Three options:
- fixed constant address, created automatically, one storage address
- queried from datastorage, allocated on demand, key->address map
- queried from datastorage, created automatically, unspecified [V]
*/

#if 0

//vfs implementation for storing in files

class FileStorage: public DataStorage, public RankStorage{

public:
};

//TODO vfs implementation for multi-level storage(MLS)

//interface to all classes that want to be in MLS
class RankStorage{

public:
};

//template<...?>
class MultiRankStorage: public DataStorage{

public:
};

//TODO vfs implementation for storing in ram

class DRAMStorage: public DataStorage, public RankStorage{

public:
};
#endif

#if 0

//Addressing raw data(DataStorage)
using ExampleMRStorage = MultiRankStorage<MemoryStorage, SSDStorage, DiskStorage, NetworkStorage>;

template <typename T...>
class MultiRankStorage{
public:
    
};

class DataStorage{
public:
    struct Stat{
        //TODO
    };
    virtual StorageRawBuffer write_raw_start(const StorageAddress &addr) = 0;
    virtual Result write_raw_finish(const StorageAddress &addr, const StorageRawBuffer &raw_buffer) = 0;

    virtual Result write(const StorageAddress &addr, const StorageBuffer &buffer) = 0;
    virtual Result erase(const StorageAddress &addr) = 0;

    virtual StorageRawBuffer read_raw_start(const StorageAddress &addr) = 0;
    virtual Result read_raw_finish(const StorageAddress &addr) = 0;
    virtual Result read(const StorageAddress &addr, const StorageBuffer &buffer) = 0;

    virtual Stat stat(const StorageAddress &addr) = 0;
};

class RankStorage{ //TODO maintain fragmented address space mapping to memory or file
public:
    virtual isAvailable(size_t size) = 0;
    virtual StorageRawBuffer getBuffer(const StorageAddress &addr) = 0;
};

class RankStorageManager{ //TODO maintain LRU or usage patterns
public:
    virtual rotate() = 0; //?
};

template <typename R1, typename R2>
class RankStorage2: public DataStorage{ //TODO maintain address space mapping to R1 and to R2, and synchronized flag
public:
    virtual StorageRawBuffer write_raw_start(const StorageAddress &addr) override;
    virtual Result write_raw_finish(const StorageAddress &addr, const StorageRawBuffer &raw_buffer) override;

    virtual Result write(const StorageAddress &addr, const StorageBuffer &buffer) override {

    }
    virtual Result erase(const StorageAddress &addr) override;

    virtual StorageRawBuffer read_raw_start(const StorageAddress &addr) override;
    virtual Result read_raw_finish(const StorageAddress &addr) override;
    virtual Result read(const StorageAddress &addr, const StorageBuffer &buffer) override;

    virtual Stat stat(const StorageAddress &addr) override;
};

template <typename T>
class DataStorage{
public:

};


class enum Result{
    Success,
    Failure,

    bool isSuccess() const{
        return *this == Success;
    }
};

struct StorageBuffer{
    void *data;
    size_t size;
};

struct StorageRawBuffer{
    //TODO things to give direct access
}

//generic virtual file system
struct StorageAddress{
    uint64_t offset;
    size_t size;
};

//Addressing raw bytes
using ExampleDataStorage = DataStorage<ExampleMRStorage>;
#endif