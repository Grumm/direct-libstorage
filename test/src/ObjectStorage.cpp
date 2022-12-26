#include "gtest/gtest.h"

#include <storage/ObjectStorage.hpp>
#include <storage/SimpleObjectStorage.hpp>
#include <storage/SimpleStorage.hpp>

namespace{

constexpr size_t MEMORYSIZE = 22;
constexpr size_t OBJSTORAGE_MEM_ALLOC = 1024;

struct TestObj{
    int a;
    //TODO make sure we use concept on this
    bool operator==(const TestObj &other) const{
        return a == other.a;
    }
};

template<template<typename, typename, size_t>typename T, typename O>
struct ParamWrapper{
    using TObjectStorage = ObjectStorage<O, T, SimpleRamStorage<MEMORYSIZE>>;
};

template<typename X>
class ObjectStorageTest: public ::testing::Test{
protected:
    using TObjectStorage = X::TObjectStorage;
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};

};

TYPED_TEST_SUITE_P(ObjectStorageTest);

TYPED_TEST_P(ObjectStorageTest, SimplePutGetCheck){
    auto base = this->storage.get_random_address(OBJSTORAGE_MEM_ALLOC);
    typename TestFixture::TObjectStorage _os(this->storage, this->storage, base);
    auto os = _os.get().get();
    
    auto index0 = os.put(TestObj{1});
    EXPECT_EQ(index0, 0);
    EXPECT_EQ(os.template get<ObjectStorageAccess::Once>(index0), TestObj{1});
}

REGISTER_TYPED_TEST_SUITE_P(ObjectStorageTest, SimplePutGetCheck);
using SimpleSimpleObjList = ParamWrapper<SimpleObjectStorage, TestObj>;
INSTANTIATE_TYPED_TEST_SUITE_P(SimpleObjectStorageSimpleTestObj, ObjectStorageTest, SimpleSimpleObjList);

}