
#include <memory>

#include <storage/UniqueID.hpp>
#include <storage/DataStorage.hpp>
#include <storage/Serialize.hpp>
#include <storage/RandomMemoryAccess.hpp>
#include <storage/SimpleStorage.hpp>

#include "gtest/gtest.h"

namespace{

class TestUIDClass: public UniqueIDInstance{
public:
    int a;
    TestUIDClass(): UniqueIDInstance(0){}
    TestUIDClass(int a, uint32_t id): UniqueIDInstance(id), a(a) {
    }

    bool operator==(const TestUIDClass &other) const{
        return a == other.a && this->getUniqueID() == other.getUniqueID();
    }
    Result serializeImpl(StorageBuffer<> &buffer) const {
        buffer.get<int>()[0] = a;
        return Result::Success;
    }
    static TestUIDClass deserializeImpl(const StorageBufferRO<> &buffer, uint32_t id) {
        TestUIDClass tuc{buffer.get<int>()[0], id};
        return tuc;
    }
    size_t getSizeImpl() const { return sizeof(a); }
};

constexpr size_t MEMORYSIZE=12;

TEST(UniqueIDTest, SimpleRegisterGet){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    {
        UniqueIDStorage<TestUIDClass, decltype(storage)> uid_s{storage};

        auto tuid1 = new TestUIDClass{7, 1};
        uid_s.registerInstance(*tuid1);
        auto &tuid1_inst = uid_s.getInstance<TestUIDClass>(1);
        EXPECT_EQ(*tuid1, tuid1_inst);
    }
}

TEST(UniqueIDTest, RegisterGetSerializeDeserializeUIDStorage){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    StorageAddress addr1;
    {
        UniqueIDStorage<TestUIDClass, decltype(storage)> uid_s{storage};

        auto tuid1 = new TestUIDClass{7, 1};
        uid_s.registerInstance(*tuid1);
        auto &tuid1_inst = uid_s.getInstance<TestUIDClass>(1);
        EXPECT_EQ(*tuid1, tuid1_inst);

        StorageAddress addr2 = serialize<StorageAddress>(storage, *tuid1);
        uid_s.registerInstanceAddress(*tuid1, addr2);
        addr1 = serialize<StorageAddress>(storage, uid_s);
    }
    {
        auto uid_s = deserialize<UniqueIDStorage<TestUIDClass, decltype(storage)>>(storage, addr1, storage);
        auto &tuid2_inst = uid_s.getInstance<TestUIDClass>(1);
        EXPECT_EQ(tuid2_inst.a, 7);
        EXPECT_EQ(tuid2_inst.UniqueIDInstance::getUniqueID(), 1);
    }
}

TEST(UniqueIDTest, DeleteInstance){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    StorageAddress addr1;
    {
        UniqueIDStorage<TestUIDClass, decltype(storage)> uid_s{storage};

        auto tuid1 = new TestUIDClass{7, 1};
        uid_s.registerInstance(*tuid1);
        auto &tuid1_inst = uid_s.getInstance<TestUIDClass>(1);
        EXPECT_EQ(*tuid1, tuid1_inst);

        StorageAddress addr2 = serialize<StorageAddress>(storage, *tuid1);
        uid_s.registerInstanceAddress(*tuid1, addr2);
        addr1 = serialize<StorageAddress>(storage, uid_s);

        uid_s.deleteInstance(tuid1_inst);
        EXPECT_THROW(uid_s.getInstance<TestUIDClass>(1), std::logic_error);
        serialize<StorageAddress>(storage, uid_s, addr1);
    }
    {
        auto uid_s = deserialize<UniqueIDStorage<TestUIDClass, decltype(storage)>>(storage, addr1, storage);
        EXPECT_THROW(uid_s.getInstance<TestUIDClass>(1), std::logic_error);
    }
}

TEST(UniqueIDTest, UniqueIDPtrSerializeDeserialize){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};

    UniqueIDStorage<TestUIDClass, decltype(storage)> uid_s{storage};

    UniqueIDPtr<TestUIDClass> uidptr1(new TestUIDClass{7, 1});
    uid_s.registerInstance(uidptr1.get());

    StorageAddress addr2 = serialize<StorageAddress>(storage, uidptr1);
    {
        auto uidptr2 = deserialize<UniqueIDPtr<TestUIDClass>>(storage, addr2, uid_s);
        EXPECT_EQ(uidptr1.get(), uidptr2.get());
        EXPECT_EQ(&uidptr1.get(), &uidptr2.get());
    }
}

}
