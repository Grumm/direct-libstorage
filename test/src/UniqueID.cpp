
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
    virtual ~TestUIDClass(){}
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
    auto rma = std::make_unique<MemoryRMA<MEMORYSIZE>>();
    auto storage = std::make_unique<SimpleRamStorage<MEMORYSIZE>>(*rma.get());
    {
        UniqueIDStorage<TestUIDClass> uid_s{*storage.get()};

        auto tuid1 = new TestUIDClass{7, 1};
        uid_s.registerInstance(*tuid1);
        auto &tuid1_inst = uid_s.getInstance<TestUIDClass>(1);
        EXPECT_EQ(*tuid1, tuid1_inst);
    }
}

TEST(UniqueIDTest, RegisterGetSerializeDeserializeUIDStorage){
    auto rma = std::make_unique<MemoryRMA<MEMORYSIZE>>();
    auto storage = std::make_unique<SimpleRamStorage<MEMORYSIZE>>(*rma.get());
    StorageAddress addr1;
    {
        UniqueIDStorage<TestUIDClass> uid_s{*storage.get()};

        auto tuid1 = new TestUIDClass{7, 1};
        uid_s.registerInstance(*tuid1);
        auto &tuid1_inst = uid_s.getInstance<TestUIDClass>(1);
        EXPECT_EQ(*tuid1, tuid1_inst);

        StorageAddress addr2 = serialize<StorageAddress>(*storage.get(), *tuid1);
        uid_s.registerInstanceAddress(tuid1->getUniqueID(), addr2);
        addr1 = serialize<StorageAddress>(*storage.get(), uid_s);
    }
    {
        auto uid_s = deserialize<UniqueIDStorage<TestUIDClass>>(*storage.get(), addr1, *storage.get());
        auto &tuid2_inst = uid_s.getInstance<TestUIDClass>(1);
        EXPECT_EQ(tuid2_inst.a, 7);
        EXPECT_EQ(tuid2_inst.UniqueIDInstance::getUniqueID(), 1);
    }
}

TEST(UniqueIDTest, DeleteInstance){
    auto rma = std::make_unique<MemoryRMA<MEMORYSIZE>>();
    auto storage = std::make_unique<SimpleRamStorage<MEMORYSIZE>>(*rma.get());
    StorageAddress addr1;
    {
        UniqueIDStorage<TestUIDClass> uid_s{*storage.get()};

        auto tuid1 = new TestUIDClass{7, 1};
        uid_s.registerInstance(*tuid1);
        auto &tuid1_inst = uid_s.getInstance<TestUIDClass>(1);
        EXPECT_EQ(*tuid1, tuid1_inst);

        StorageAddress addr2 = serialize<StorageAddress>(*storage.get(), *tuid1);
        uid_s.registerInstanceAddress(tuid1->getUniqueID(), addr2);
        addr1 = serialize<StorageAddress>(*storage.get(), uid_s);

        uid_s.deleteInstance(tuid1_inst);
        EXPECT_THROW(uid_s.getInstance<TestUIDClass>(1), std::logic_error);
        serialize<StorageAddress>(*storage.get(), uid_s, addr1);
    }
    {
        auto uid_s = deserialize<UniqueIDStorage<TestUIDClass>>(*storage.get(), addr1, *storage.get());
        EXPECT_THROW(uid_s.getInstance<TestUIDClass>(1), std::logic_error);
    }
}

}