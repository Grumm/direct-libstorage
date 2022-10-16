#include "gtest/gtest.h"

#include <storage/SimpleObjectStorage.hpp>
#include <storage/SimpleStorage.hpp>

namespace{

constexpr size_t MEMORYSIZE = 10;
constexpr size_t OBJSTORAGE_MEM_ALLOC = 1024;

struct TestObj{
    int a;
    //TODO make sure we use concept on this
    bool operator==(const TestObj &other) const{
        return a == other.a;
    }
};

TEST(SimpleObjectStorageTest, SimplePutGetCheck){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    auto base = storage.get_random_address(OBJSTORAGE_MEM_ALLOC);
    SimpleObjectStorage<TestObj, decltype(storage)> os(storage, base);
    
    auto index0 = os.put(TestObj{1});
    EXPECT_EQ(index0, 0);
    EXPECT_EQ(os.get(index0, ObjectStorageAccess::Once), TestObj{1});
}

TEST(SimpleObjectStorageTest, SimplePutGetSerialize){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    auto static_section = storage.get_static_section();
    auto base = storage.get_random_address(OBJSTORAGE_MEM_ALLOC);
    serialize<StorageAddress>(storage, base, static_section);
    StorageAddress os_addr;
    {
        SimpleObjectStorage<TestObj, decltype(storage)> os(storage, base);
        
        auto index0 = os.put(TestObj{1});

        os_addr = serialize<StorageAddress>(storage, os);
    }
    {
        auto os = deserialize<SimpleObjectStorage<TestObj, decltype(storage)>>(storage, os_addr, storage);

        EXPECT_EQ(os.get(0, ObjectStorageAccess::Once), TestObj{1});
    }
}

TEST(SimpleObjectStorageTest, MultiplePutGetCheck){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    auto base = storage.get_random_address(OBJSTORAGE_MEM_ALLOC);
    SimpleObjectStorage<TestObj, decltype(storage)> os(storage, base);
    
    auto index0 = os.put(TestObj{1});
    auto index1 = os.put(TestObj{5});
    EXPECT_EQ(index0, 0);
    EXPECT_EQ(index1, 1);
    EXPECT_EQ(os.get(index0, ObjectStorageAccess::Once), TestObj{1});
    auto index2 = os.put(TestObj{10});
    EXPECT_EQ(index2, 2);
    EXPECT_EQ(os.get(index1, ObjectStorageAccess::Once), TestObj{5});
    EXPECT_EQ(os.get(index2, ObjectStorageAccess::Once), TestObj{10});
}

TEST(SimpleObjectStorageTest, MultipleSerializeDeserialize){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    auto base = storage.get_random_address(OBJSTORAGE_MEM_ALLOC);
    StorageAddress os_addr;
    
    {
        SimpleObjectStorage<TestObj, decltype(storage)> os(storage, base);
        
        auto index0 = os.put(TestObj{1});
        auto index1 = os.put(TestObj{5});
        auto index2 = os.put(TestObj{10});

        os_addr = serialize<StorageAddress>(storage, os);
    }
    {
        auto os = deserialize<SimpleObjectStorage<TestObj, decltype(storage)>>(storage, os_addr, storage);

        EXPECT_EQ(os.get(0, ObjectStorageAccess::Once), TestObj{1});
        EXPECT_EQ(os.get(1, ObjectStorageAccess::Once), TestObj{5});
        EXPECT_EQ(os.get(2, ObjectStorageAccess::Once), TestObj{10});
    }
}

}