#pragma once

#include <concepts>

#include <storage/Utils.hpp>
#include <storage/StorageUtils.hpp>
#include <storage/ObjectInstanceStorage.hpp>

//where to store all objects, without moving existing objects(which will invalidate runtime)
template<size_t TSize, CStorageImpl Storage, AllocationPattern A, size_t MAX_OBJS>
class ObjectAddressStorage{
    Storage &storage;

    std::vector<StorageAddress> buckets;
    static constexpr size_t DEFAULT_ENTRIES = (1 << 4); //16
public:
    ObjectAddressStorage(Storage &storage): storage(storage) {
        //get(DEFAULT_ENTRIES - 1);
    }

    //get address from existing base, or allocate new
    StorageAddress get(size_t index) {
        ASSERT_ON(index >= MAX_OBJS);
        //get most significant bit. e.g. 3 -> 0b11 -> 0b10 -> index 1 -> bucket 1
        auto bucket = MostSignificantBitSet(index);
        //check if bucket doesn't exist. if not, fill all buckets up to this one
        if(bucket >= buckets.size()){
            for(size_t i = buckets.size(); i <= bucket; i++){
                size_t count = (1 << i);
                buckets.push_back(storage.get_random_address(count * TSize))
            }
        }
        auto base = buckets[bucket];
        auto offset = index & ~(1 << bucket);
        return base.subrange(offset * TSize, TSize);
    }
    void clear(){
        for(size_t i = 0; i < buckets.size(); i++){
            storage.erase(buckets[i]);
        }
        buckets.clear();
        //get(DEFAULT_ENTRIES - 1);
    }
    //TODO serialize impl
};

//TOOD implement random and range trackers
template<AllocationPattern A, size_t MAX_OBJS>
requires std::is_same<A, AllocationPattern::Sequential>
class ObjectAllocationTracker{
    /*struct AllocatedBucketL{
        size_t offset;
        size_t size;
        std::vector<bool> bits;
        std::vector<AllocatedBucketL> allocated;
    };*/
    std::vector<bool> allocated;
public:
    void allocate(size_t index){
        if(index >= allocated.size()){
            allocated.resize(index + 1);
        }
        allocated[index] = true;
    }
    void deallocate(size_t index){
        ASSERT_ON(index >= allocated.size());
        allocated[index] = false;
    }
    template<typename F>
    void for_each(F &f) const{
        for(size_t i = 0; i < allocated.size(); i++){
            if(allocated[i]){
                f(i);
            }
        }
    }
    void clear(){
        allocated.clear();
    }
    //TODO serialize impl
};

//TODO special case for trivially copyable to instantiate at the memory with const or not
//TODO specialize MAX_OBJS in range < 1K, < 10K etc.

template<CSerializable T, CStorageImpl Storage, AllocationPattern A, size_t MAX_OBJS>
requires CSerializableFixedSize<T>
class ObjectStorageImpl{
    static constexpr size_t TSize = sizeof(T);

    Storage &storage;
    ObjectAddressStorage<TSize, Storage, A, MAX_OBJS> addr_storage;
    ObjectInstanceStorage<T, A, MAX_OBJS> obj_storage;
    ObjectAllocationTracker<A, MAX_OBJS> obj_alloc;

    size_t obj_count{0};
public:
    ObjectStorageImpl(){}

    //get instance
    T &get(size_t index, bool nocache = false){ //question is how long do we keep the reference if nocache = true. maybe list of pending operations
    }

    //delete instance
    void clear(size_t index){
    }

    //add object at index
    void put(auto t, size_t index){
        ASSERT_ON(obj_count >= MAX_OBJS); //no more, full TODO maybe return normal error
        if(addr.size() <= index){
            addr.resize(index + 1);
        }
        auto &a = addr[index];
        if(!a.is_null()){
            //do something with old object, like destroy instance etc.
        }
        //serialize object
        a = serialize<StorageAddress>(t, storage, a.second);
    }

    //add object, return index
    size_t put(auto t){
        ASSERT_ON(obj_count >= MAX_OBJS); //no more, full TODO maybe return normal error
    }

    //delete object
    void del(size_t index){
    }

    //has object
    bool has(size_t index) const{

    }
//TODO serialize etc.
};

/*
template<typename T>
class SimpleObjectStorage{
public:
    SimpleObjectStorage(Storage &storage, StorageAddress base) {}
    constexpr ObjectStorageAccess DefaultAccess = ObjectStorageAccess::Once;
    bool has(size_t index) const {

    }
    size_t size() const {
        return states[index]
    }
    T get(size_t index, ObjectStorageAccess access){

    }
    T &getRef(size_t index, ObjectStorageAccess access){

    }
    const T &getCRef(size_t index, ObjectStorageAccess access){

    }
    void clear(size_t start, size_t end){

    }
    size_t put(const T &t){
        
    }
    void put(const T &t, size_t index,){
        
    }
    size_t put(const T &&t, ObjectStorageAccess access){
        
    }
    void put(const T &&t, size_t index, ObjectStorageAccess access){
        
    }
};
*/

/***************************/

#if 0
template<CSerializable T, size_t MAX_OBJS>
class GenericObjectStorageImpl{
    std::vector<T> objs;
public:
    GenericObjectStorageImpl(){}
    const T &get(size_t index) const{
        ASSERT_ON(index >= objs.size());
        return objs[index];
    }
    T &get(size_t index) {
        ASSERT_ON(index >= objs.size());
        return objs[index];
    }
    void put(const T &t, size_t index){
        ASSERT_ON(t.is_null());
        if(index >= objs.size()){
            objs.resize(index + 1);
        }
        objs[index] = t;
    }
    size_t add(const T &t){
        ASSERT_ON(t.is_null());
        objs.push_back(t);
        return objs.size() - 1;
    }
    void del(size_t index){
        ASSERT_ON(index >= objs.size());
        objs[index].reset();
    }
};

#if 0
template <size_t N, size_t M, size_t K>
struct compare_size{
    using value = std::if<
}

template<CSerializable T, size_t MAX_OBJS>
requires
class GenericObjectStorageImpl{
    std::array<T, MAX_OBJS> objs;
public:
    GenericObjectStorageImpl(){}
    const T &get(size_t index) const{
        ASSERT_ON(index >= objs.size());
        return objs[index];
    }
    T &get(size_t index) {
        ASSERT_ON(index >= objs.size());
        return objs[index];
    }
    void put(const T &t, size_t index){
        ASSERT_ON(t.is_null());
        if(index >= objs.size()){
            objs.resize(index + 1);
        }
        objs[index] = t;
    }
    size_t add(const T &t){
        ASSERT_ON(t.is_null());
        objs.push_back(t);
        return objs.size() - 1;
    }
    void del(size_t index){
        ASSERT_ON(index >= objs.size());
        objs[index].reset();
    }
};
#endif

template<CStorageImpl Storage, size_t MAX_OBJS = std::numeric_limits<uint8_t>::max()>
class GenericObjectStorage{
    Storage &storage;
    StaticHeader header;

    static constexpr uint32_t OS_MAGIC = 0xF4E570F3;
    static constexpr size_t DEFAULT_ALLOC_SIZE = (1 << 32); //4GB

    void init_gobjstorage(){
        ASSERT_ON(header.size < szeimpl::size(StaticHeader<T>{}));
        if(header.magic != OS_MAGIC){
            //new
            header.magic = OS_MAGIC;
            header.address = storage.template get_random_address(DEFAULT_ALLOC_SIZE);
        } else {
            //deserialize mlkm
            obj_storage_impl = deserialize<ObjectStorageImpl<T, Storage, MAX_OBJS>>(storage, header.address);
        }
    }
public:
    GenericObjectStorage(Storage &storage, StorageAddress &addr): 
        storage(storage),
        header_address(addr),
        header(deserialize<StaticHeader<T>>(storage, header_address)) {
            init_gobjstorage();
    }
};
#endif


#if 0
//vector that keeps iterators valid in insert/
template<typename T>
class PersistentVector{
public:
    class iterator



};
//probably not needed
template<CSerializable T>
class ObjectAddress{
    StorageAddress addr;
    size_t index;
public:
    size_t size() const{
        return addr.size;
    }
};
template<typename T, size_t MAX_OBJS>
class ObjectInstanceRange{
    size_t offset;
    std::deque<T> instance_vector; //TODO persistent vector, maybe just deque
public:
    ObjectInstanceRange(size_t offset): offset(offset) {}

    void add(size_t index, T &&t){

    }

    void del(size_t index){

    }

    T &get(size_t index) const{

    }
};
#endif