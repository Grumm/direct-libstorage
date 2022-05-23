
#include <cstdio>
#include <memory>

#include <storage/RandomMemoryAccess.hpp>
#include <storage/SimpleStorage.hpp>
#include <storage/Serialize.hpp>

#include "gtest/gtest.h"


namespace{

constexpr size_t MEMORYSIZE=12;

class SerializableImplTest;

class SerializableImplTest{
public:
    std::map<int, int> m;
    SerializableImplTest(int a, int b){
        m[a] = b;
    }
    SerializableImplTest(const SerializableImplTest &other){
        m = other.m;
    }

    Result serializeImpl(StorageBuffer<> &buffer) const {
        int *i = buffer.get<int>();
        i[0] = m.begin()->first;
        i[1] = m.begin()->second;
        return Result::Success;
    }
    static SerializableImplTest deserializeImpl(const StorageBufferRO<> &buffer) {
        const int *i = buffer.get<int>();
        return SerializableImplTest{i[0], i[1]};
    }
    size_t getSizeImpl() const {
        return sizeof(int) * 2;
    }

    bool operator==(const SerializableImplTest &other) const{
        bool ret = true;
        ret = ret && (m.size() == other.m.size());
        ret = ret && (m.begin()->first == other.m.begin()->first);
        ret = ret && (m.begin()->second == other.m.begin()->second);
        return ret;
    }
};

const std::string filename{"/tmp/FileRMA.test"};

TEST(SerializeTest, SimpleSerializeDeserialize){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};

    SerializableImplTest si{1, 2};
    StorageAddress addr = serialize<StorageAddress>(storage, si);
    SerializableImplTest si_res = deserialize<SerializableImplTest>(storage, addr);
    EXPECT_EQ(si, si_res);
}

TEST(SerializeTest, SimpleOverwrite){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};

    SerializableImplTest si{1, 2};
    SerializableImplTest si2{3, 8};
    StorageAddress addr;
    serialize<StorageAddress>(storage, si2, addr);
    auto si2_res = deserialize_ptr<SerializableImplTest>(storage, addr);
    EXPECT_EQ(si2, *si2_res.get());
    //overwrite
    serialize<StorageAddress>(storage, si, addr);
    auto si2_res2 = deserialize<SerializableImplTest>(storage, addr);
    EXPECT_EQ(si, si2_res2);
}

TEST(SerializeTest, IntegralSerializeDeserialize){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};

    int a = 3;
    StorageAddress addr = serialize<StorageAddress>(storage, a);
    int a_res = deserialize<int>(storage, addr);
    EXPECT_EQ(a, a_res);
}

TEST(SerializeTest, IntegralOverwrite){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};

    int a1 = 5;
    int a2 = -2;
    StorageAddress addr;
    serialize<StorageAddress>(storage, a1, addr);
    auto a1_res = deserialize_ptr<int>(storage, addr);
    EXPECT_EQ(*a1_res, a1);
    //overwrite
    serialize<StorageAddress>(storage, a2, addr);
    auto a2_res = deserialize<int>(storage, addr);
    EXPECT_EQ(a2_res, a2);
}

TEST(SerializeTest, StringSerializeDeserialize){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
    {
        std::string s{"string1"};
        StorageAddress addr = serialize<StorageAddress>(storage, s);
        std::string s_res = deserialize<std::string>(storage, addr);
        EXPECT_EQ(s, s_res);
    }
    {
        std::string s{""};
        StorageAddress addr = serialize<StorageAddress>(storage, s);
        std::string s_res = deserialize<std::string>(storage, addr);
        EXPECT_EQ(s, s_res);
    }
}

TEST(SerializeTest, StringOverwrite){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};

    std::string s1{"string1"};
    std::string s2{"2s0"};
    StorageAddress addr;
    serialize<StorageAddress>(storage, s1, addr);
    auto s1_res = deserialize_ptr<std::string>(storage, addr);
    EXPECT_EQ(*s1_res, s1);
    //overwrite
    serialize<StorageAddress>(storage, s2, addr);
    auto s2_res = deserialize<std::string>(storage, addr);
    EXPECT_EQ(s2_res, s2);
}

struct TestPOD{
    int a;
    uint64_t b;
    bool c;
    enum E{
        e1,
        e2,
    };
    E e;
    bool operator==(const TestPOD &other) const{
        return (a == other.a) && (b == other.b) && (c == other.c) && (e == other.e);
    }
};

TEST(SerializeTest, PODSerializeDeserialize){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};

    TestPOD pod{6, 444444, true, TestPOD::E::e2};
    StorageAddress addr = serialize<StorageAddress>(storage, pod);
    TestPOD pod_res = deserialize<TestPOD>(storage, addr);
    EXPECT_EQ(pod, pod_res);
}

TEST(SerializeTest, PODOverwrite){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};

    TestPOD pod1{6, 444444, true, TestPOD::E::e2};
    TestPOD pod2{-11, 3, false, TestPOD::E::e1};
    StorageAddress addr;
    serialize<StorageAddress>(storage, pod1, addr);
    auto pod1_res = deserialize_ptr<TestPOD>(storage, addr);
    EXPECT_EQ(*pod1_res.get(), pod1);
    //overwrite
    serialize<StorageAddress>(storage, pod2, addr);
    auto pod2_res = deserialize<TestPOD>(storage, addr);
    EXPECT_EQ(pod2, pod2_res);
}

TEST(SerializeTest, SimplePairSerializeDeserialize){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};

    std::pair<int, const uint64_t> pair1{-5, 0xfffffE6};
    StorageAddress addr = serialize<StorageAddress>(storage, pair1);
    auto pair_res = deserialize<decltype(pair1)>(storage, addr);
    EXPECT_EQ(pair_res.first, -5);
    EXPECT_EQ(pair_res.second, 0xfffffE6);
}

TEST(SerializeTest, ComplexPairSerializeDeserialize){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};

    TestPOD pod1{6, 444444, true, TestPOD::E::e2};
    SerializableImplTest si1{13, -2};
    std::pair<const std::pair<TestPOD, std::string>,
        std::pair<const uint64_t, SerializableImplTest>> pair1{{pod1, "t1anyways"}, {111, si1}};
    StorageAddress addr = serialize<StorageAddress>(storage, pair1);
    auto pair_res = deserialize<decltype(pair1)>(storage, addr);
    auto &pod_res = pair_res.first.first;
    auto &str_res = pair_res.first.second;
    auto &u64_res = pair_res.second.first;
    auto &si_res = pair_res.second.second;
    EXPECT_EQ(pod_res, pod1);
    EXPECT_EQ(str_res, "t1anyways");
    EXPECT_EQ(u64_res, 111UL);
    EXPECT_EQ(si_res, si1);
}

TEST(SerializeTest, SimpleTupleSerializeDeserialize){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};

    std::tuple<int, uint64_t, const char> tuple1{-333, 0xeeef6, 'L'};
    StorageAddress addr = serialize<StorageAddress>(storage, tuple1);
    auto tuple_res = deserialize<decltype(tuple1)>(storage, addr);
    EXPECT_EQ(std::get<0>(tuple_res), -333);
    EXPECT_EQ(std::get<1>(tuple_res), 0xeeef6);
    EXPECT_EQ(std::get<2>(tuple_res), 'L');
}

TEST(SerializeTest, ComplexTupleSerializeDeserialize){
    SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};

    SerializableImplTest si1{13, -2};
    TestPOD pod1{6, 444444, true, TestPOD::E::e2};
    std::tuple<SerializableImplTest, uint64_t,
        const std::pair<const char, TestPOD>> tuple1{si1, 0xeeef6, {'W', pod1}};
    StorageAddress addr = serialize<StorageAddress>(storage, tuple1);
    auto tuple_res = deserialize<decltype(tuple1)>(storage, addr);
    EXPECT_EQ(std::get<0>(tuple_res).m[13], -2);
    EXPECT_EQ(std::get<1>(tuple_res), 0xeeef6);
    EXPECT_EQ(std::get<2>(tuple_res).first, 'W');
    auto &pod_res = std::get<2>(tuple_res).second;
    EXPECT_EQ(pod_res, pod1);
}

}
