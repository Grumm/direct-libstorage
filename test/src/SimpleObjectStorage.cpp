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
    EXPECT_EQ(os.get<ObjectStorageAccess::Once>(index0), TestObj{1});
}

TEST(SimpleObjectStorageTest, SimplePutGetRef){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    auto base = storage.get_random_address(OBJSTORAGE_MEM_ALLOC);
    SimpleObjectStorage<TestObj, decltype(storage)> os(storage, base);
    
    auto index0 = os.put(TestObj{1});
    auto &v1 = os.getRef(index0);
    EXPECT_EQ(v1, TestObj{1});
    v1.a = 3;
    const auto &v1c = os.getCRef(index0);
    EXPECT_EQ(v1c, TestObj{3});
    EXPECT_EQ(v1c, v1);
}

TEST(SimpleObjectStorageTest, SimplePutIndexGet){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    auto base = storage.get_random_address(OBJSTORAGE_MEM_ALLOC);
    SimpleObjectStorage<TestObj, decltype(storage)> os(storage, base);
    
    size_t index0 = 0;
    os.put(TestObj{1}, index0);
    EXPECT_TRUE(os.has(index0));
    EXPECT_EQ(os.get<ObjectStorageAccess::Once>(index0), TestObj{1});
}

TEST(SimpleObjectStorageTest, MultiplePutIndexGet){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    auto base = storage.get_random_address(OBJSTORAGE_MEM_ALLOC);
    SimpleObjectStorage<TestObj, decltype(storage)> os(storage, base);
    
    size_t index0 = 0;
    size_t index1 = 1;
    os.put(TestObj{1}, index0);
    os.put(TestObj{5}, index1);
    EXPECT_TRUE(os.has(index0));
    EXPECT_TRUE(os.has(index1));
    EXPECT_EQ(os.get<ObjectStorageAccess::Once>(index0), TestObj{1});
    EXPECT_EQ(os.get<ObjectStorageAccess::Once>(index1), TestObj{5});
}

TEST(SimpleObjectStorageTest, MultiplePutMixedGet){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    auto base = storage.get_random_address(OBJSTORAGE_MEM_ALLOC);
    SimpleObjectStorage<TestObj, decltype(storage)> os(storage, base);
    
    size_t index0 = 0;
    size_t index1 = 1;
    os.put(TestObj{1}, index0);
    os.put(TestObj{5}, index1);
    auto index2 = os.put(TestObj{10});
    EXPECT_EQ(index2, 2);
    EXPECT_TRUE(os.has(index0));
    EXPECT_TRUE(os.has(index1));
    EXPECT_TRUE(os.has(index2));
    EXPECT_EQ(os.get<ObjectStorageAccess::Once>(index0), TestObj{1});
    EXPECT_EQ(os.get<ObjectStorageAccess::Once>(index1), TestObj{5});
    EXPECT_EQ(os.get<ObjectStorageAccess::Once>(index2), TestObj{10});
}

TEST(SimpleObjectStorageTest, SimplePutRValueGet){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    auto base = storage.get_random_address(OBJSTORAGE_MEM_ALLOC);
    SimpleObjectStorage<TestObj, decltype(storage)> os(storage, base);
    
    TestObj to1{1};
    auto index0 = os.put(std::move(to1));
    EXPECT_EQ(index0, 0);
    EXPECT_EQ(os.get<ObjectStorageAccess::Once>(index0), TestObj{1});
}

TEST(SimpleObjectStorageTest, SimplePutOverwriteGet){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    auto base = storage.get_random_address(OBJSTORAGE_MEM_ALLOC);
    SimpleObjectStorage<TestObj, decltype(storage)> os(storage, base);
    
    auto index0 = os.put(TestObj{1});
    os.put(TestObj{5}, index0);
    EXPECT_TRUE(os.has(index0));
    EXPECT_EQ(os.get<ObjectStorageAccess::Once>(index0), TestObj{5});
    os.clear(index0);
    EXPECT_FALSE(os.has(index0));
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

        EXPECT_EQ(os.get<ObjectStorageAccess::Once>(0), TestObj{1});
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
    EXPECT_TRUE(os.has(index0));
    EXPECT_TRUE(os.has(index1));
    EXPECT_FALSE(os.has(2));
    EXPECT_EQ(os.get<ObjectStorageAccess::Once>(index0), TestObj{1});
    auto index2 = os.put(TestObj{10});
    EXPECT_EQ(index2, 2);
    EXPECT_EQ(os.get<ObjectStorageAccess::Once>(index1), TestObj{5});
    EXPECT_EQ(os.get<ObjectStorageAccess::Once>(index2), TestObj{10});
    EXPECT_TRUE(os.has(index0));
    EXPECT_TRUE(os.has(index1));
    EXPECT_TRUE(os.has(index2));
}

TEST(SimpleObjectStorageTest, SimplePutClear){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    auto base = storage.get_random_address(OBJSTORAGE_MEM_ALLOC);
    SimpleObjectStorage<TestObj, decltype(storage)> os(storage, base);
    
    auto index0 = os.put(TestObj{1});
    os.clear(index0);
    EXPECT_FALSE(os.has(index0));
}

TEST(SimpleObjectStorageTest, MultiplePutClear){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    auto base = storage.get_random_address(OBJSTORAGE_MEM_ALLOC);
    SimpleObjectStorage<TestObj, decltype(storage)> os(storage, base);
    
    auto index0 = os.put(TestObj{1});
    auto index1 = os.put(TestObj{5});
    auto index2 = os.put(TestObj{10});
    os.clear(index1);
    EXPECT_TRUE(os.has(index0));
    EXPECT_FALSE(os.has(index1));
    EXPECT_TRUE(os.has(index2));
    os.clear(index0);
    EXPECT_FALSE(os.has(index0));
    EXPECT_FALSE(os.has(index1));
    EXPECT_TRUE(os.has(index2));
    os.clear(index2);
    EXPECT_FALSE(os.has(index0));
    EXPECT_FALSE(os.has(index1));
    EXPECT_FALSE(os.has(index2));
}

TEST(SimpleObjectStorageTest, MultiplePutClearRange){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    auto base = storage.get_random_address(OBJSTORAGE_MEM_ALLOC);
    SimpleObjectStorage<TestObj, decltype(storage)> os(storage, base);
    
    auto index0 = os.put(TestObj{1});
    auto index1 = os.put(TestObj{5});
    auto index2 = os.put(TestObj{10});
    auto index3 = os.put(TestObj{20});
    auto index4 = os.put(TestObj{30});
    os.clear(index1, index3);
    EXPECT_TRUE(os.has(index0));
    EXPECT_FALSE(os.has(index1));
    EXPECT_FALSE(os.has(index2));
    EXPECT_FALSE(os.has(index3));
    EXPECT_TRUE(os.has(index4));
    os.clear(index0, index4 + 1);
    EXPECT_FALSE(os.has(index0));
    EXPECT_FALSE(os.has(index1));
    EXPECT_FALSE(os.has(index2));
    EXPECT_FALSE(os.has(index3));
    EXPECT_FALSE(os.has(index4));
    EXPECT_FALSE(os.has(index4 + 1));
}

TEST(SimpleObjectStorageTest, MultiplePutClearManyTimes){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    auto base = storage.get_random_address(OBJSTORAGE_MEM_ALLOC);
    SimpleObjectStorage<TestObj, decltype(storage)> os(storage, base);
    
    auto index0 = os.put(TestObj{1});
    auto index1 = os.put(TestObj{5});
    auto index2 = os.put(TestObj{10});
    os.clear(index1);
    auto index3 = os.put(TestObj{20});
    auto index4 = os.put(TestObj{30});
    os.clear(index3);
    os.clear(index0);
    auto index5 = os.put(TestObj{40});

    EXPECT_FALSE(os.has(index0));
    EXPECT_FALSE(os.has(index1));
    EXPECT_EQ(os.get<ObjectStorageAccess::Once>(index2), TestObj{10});
    EXPECT_FALSE(os.has(index3));
    EXPECT_EQ(os.get<ObjectStorageAccess::Once>(index4), TestObj{30});
    EXPECT_EQ(os.get<ObjectStorageAccess::Once>(index5), TestObj{40});
}

TEST(SimpleObjectStorageTest, MultiplePutClearRangeManyTimes){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    auto base = storage.get_random_address(OBJSTORAGE_MEM_ALLOC);
    SimpleObjectStorage<TestObj, decltype(storage)> os(storage, base);
    
    auto index0 = os.put(TestObj{1});
    auto index1 = os.put(TestObj{5});
    auto index2 = os.put(TestObj{10});
    os.clear(index1);
    auto index3 = os.put(TestObj{20});
    auto index4 = os.put(TestObj{30});
    os.clear(index3, index4 + 1);
    auto index5 = os.put(TestObj{40});

    EXPECT_EQ(os.get<ObjectStorageAccess::Once>(index0), TestObj{1});
    EXPECT_FALSE(os.has(index1));
    EXPECT_EQ(os.get<ObjectStorageAccess::Once>(index2), TestObj{10});
    EXPECT_FALSE(os.has(index3));
    EXPECT_FALSE(os.has(index4));
    EXPECT_EQ(os.get<ObjectStorageAccess::Once>(index5), TestObj{40});
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

        EXPECT_EQ(os.get<ObjectStorageAccess::Once>(0), TestObj{1});
        EXPECT_EQ(os.get<ObjectStorageAccess::Once>(1), TestObj{5});
        EXPECT_EQ(os.get<ObjectStorageAccess::Once>(2), TestObj{10});
    }
}

TEST(SimpleObjectStorageTest, MultiplePutClearOnceSerializeDeserialize){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    auto base = storage.get_random_address(OBJSTORAGE_MEM_ALLOC);
    StorageAddress os_addr;
    
    {
        SimpleObjectStorage<TestObj, decltype(storage)> os(storage, base);
        
        auto index0 = os.put(TestObj{1});
        auto index1 = os.put(TestObj{5});
        auto index2 = os.put(TestObj{10});
        os.clear(index1);
        auto index3 = os.put(TestObj{20});
        auto index4 = os.put(TestObj{30});

        os_addr = serialize<StorageAddress>(storage, os);
    }
    {
        auto os = deserialize<SimpleObjectStorage<TestObj, decltype(storage)>>(storage, os_addr, storage);

        EXPECT_EQ(os.get<ObjectStorageAccess::Once>(0), TestObj{1});
        EXPECT_FALSE(os.has(1));
        EXPECT_EQ(os.get<ObjectStorageAccess::Once>(2), TestObj{10});
        EXPECT_EQ(os.get<ObjectStorageAccess::Once>(3), TestObj{20});
        EXPECT_EQ(os.get<ObjectStorageAccess::Once>(4), TestObj{30});
    }
}

TEST(SimpleObjectStorageTest, MultiplePutClearTwiceSerializeDeserialize){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    auto base = storage.get_random_address(OBJSTORAGE_MEM_ALLOC);
    StorageAddress os_addr;
    
    {
        SimpleObjectStorage<TestObj, decltype(storage)> os(storage, base);
        
        auto index0 = os.put(TestObj{1});
        auto index1 = os.put(TestObj{5});
        auto index2 = os.put(TestObj{10});
        os.clear(index1);
        auto index3 = os.put(TestObj{20});
        auto index4 = os.put(TestObj{30});
        os.clear(index3);
        os.clear(index0);
        auto index5 = os.put(TestObj{40});

        os_addr = serialize<StorageAddress>(storage, os);
    }
    {
        auto os = deserialize<SimpleObjectStorage<TestObj, decltype(storage)>>(storage, os_addr, storage);

        EXPECT_FALSE(os.has(0));
        EXPECT_FALSE(os.has(1));
        EXPECT_EQ(os.get<ObjectStorageAccess::Once>(2), TestObj{10});
        EXPECT_FALSE(os.has(3));
        EXPECT_EQ(os.get<ObjectStorageAccess::Once>(4), TestObj{30});
        EXPECT_EQ(os.get<ObjectStorageAccess::Once>(5), TestObj{40});
    }
}

//TODO wrapper for template concept, not implementation
//TODO access modificators check, especially ::Keep

}