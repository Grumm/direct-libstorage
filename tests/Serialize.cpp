
#include <Serialize.hpp>
#include <SimpleStorage.hpp>
#include <RandomMemoryAccess.hpp>

#include <cstdio>
#include <memory>

#include "gtest/gtest.h"


namespace{

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

    Result serializeImpl(const StorageBuffer<> &buffer) const {
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
};

const std::string filename{"/tmp/FileRMA.test"};

TEST(SerializeTest, SimpleSerializeDeserialize){
	std::unique_ptr<RMAInterface> rma{new MemoryRMA<20>()};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	SerializableImplTest si{1, 2};
	StorageAddress addr = serialize<StorageAddress>(*storage.get(), si);
	SerializableImplTest si_res = deserialize<SerializableImplTest>(*storage.get(), addr);
	ASSERT_EQ(si.m[1], si_res.m[1]);
}

TEST(SerializeTest, SimpleOverwrite){
	std::unique_ptr<RMAInterface> rma{new MemoryRMA<20>()};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	SerializableImplTest si{1, 2};
	SerializableImplTest si2{3, 8};
	StorageAddress addr;
	serialize<StorageAddress>(*storage.get(), si2, addr);
	auto si2_res = deserialize_ptr<SerializableImplTest>(*storage.get(), addr);
	ASSERT_EQ(si2.m[3], si2_res->m[3]);
	//overwrite
	serialize<StorageAddress>(*storage.get(), si, addr);
	auto si2_res2 = deserialize<SerializableImplTest>(*storage.get(), addr);
	ASSERT_EQ(si.m[1], si2_res2.m[1]);
}

TEST(SerializeTest, IntegralSerializeDeserialize){
	std::unique_ptr<RMAInterface> rma{new MemoryRMA<20>()};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	int a = 3;
	StorageAddress addr = serialize<StorageAddress>(*storage.get(), a);
	int a_res = deserialize<int>(*storage.get(), addr);
	ASSERT_EQ(a, a_res);
}

TEST(SerializeTest, IntegralOverwrite){
	std::unique_ptr<RMAInterface> rma{new MemoryRMA<20>()};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	int a1 = 5;
	int a2 = -2;
	StorageAddress addr;
	serialize<StorageAddress>(*storage.get(), a1, addr);
	auto a1_res = deserialize_ptr<int>(*storage.get(), addr);
	ASSERT_EQ(*a1_res, a1);
	//overwrite
	serialize<StorageAddress>(*storage.get(), a2, addr);
	auto a2_res = deserialize<int>(*storage.get(), addr);
	ASSERT_EQ(a2_res, a2);
}

TEST(SerializeTest, StringSerializeDeserialize){
	std::unique_ptr<RMAInterface> rma{new MemoryRMA<20>()};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};
	{
		std::string s{"string1"};
		StorageAddress addr = serialize<StorageAddress>(*storage.get(), s);
		std::string s_res = deserialize<std::string>(*storage.get(), addr);
		ASSERT_EQ(s, s_res);
	}
	{
		std::string s{""};
		StorageAddress addr = serialize<StorageAddress>(*storage.get(), s);
		std::string s_res = deserialize<std::string>(*storage.get(), addr);
		ASSERT_EQ(s, s_res);
	}
}

TEST(SerializeTest, StringOverwrite){
	std::unique_ptr<RMAInterface> rma{new MemoryRMA<20>()};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	std::string s1{"string1"};
	std::string s2{"2s0"};
	StorageAddress addr;
	serialize<StorageAddress>(*storage.get(), s1, addr);
	auto s1_res = deserialize_ptr<std::string>(*storage.get(), addr);
	ASSERT_EQ(*s1_res, s1);
	//overwrite
	serialize<StorageAddress>(*storage.get(), s2, addr);
	auto s2_res = deserialize<std::string>(*storage.get(), addr);
	ASSERT_EQ(s2_res, s2);
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
};

TEST(SerializeTest, PODSerializeDeserialize){
	std::unique_ptr<RMAInterface> rma{new MemoryRMA<20>()};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	TestPOD pod{6, 444444, true, TestPOD::E::e2};
	StorageAddress addr = serialize<StorageAddress>(*storage.get(), pod);
	TestPOD pod_res = deserialize<TestPOD>(*storage.get(), addr);
	ASSERT_EQ(pod.a, pod_res.a);
	ASSERT_EQ(pod.b, pod_res.b);
	ASSERT_EQ(pod.c, pod_res.c);
	ASSERT_EQ(pod.e, pod_res.e);
}

TEST(SerializeTest, PODOverwrite){
	std::unique_ptr<RMAInterface> rma{new MemoryRMA<20>()};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	TestPOD pod1{6, 444444, true, TestPOD::E::e2};
	TestPOD pod2{-11, 3, false, TestPOD::E::e1};
	StorageAddress addr;
	serialize<StorageAddress>(*storage.get(), pod1, addr);
	auto pod1_res = deserialize_ptr<TestPOD>(*storage.get(), addr);
	ASSERT_EQ(pod1_res->a, pod1.a);
	ASSERT_EQ(pod1_res->b, pod1.b);
	ASSERT_EQ(pod1_res->c, pod1.c);
	ASSERT_EQ(pod1_res->e, pod1.e);
	//overwrite
	serialize<StorageAddress>(*storage.get(), pod2, addr);
	auto pod2_res = deserialize<TestPOD>(*storage.get(), addr);
	ASSERT_EQ(pod2.a, pod2_res.a);
	ASSERT_EQ(pod2.b, pod2_res.b);
	ASSERT_EQ(pod2.c, pod2_res.c);
	ASSERT_EQ(pod2.e, pod2_res.e);
}

TEST(SerializeTest, SimplePairSerializeDeserialize){
	std::unique_ptr<RMAInterface> rma{new MemoryRMA<20>()};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	std::pair<int, const uint64_t> pair1{-5, 0xfffffE6};
	StorageAddress addr = serialize<StorageAddress>(*storage.get(), pair1);
	auto pair_res = deserialize<decltype(pair1)>(*storage.get(), addr);
	ASSERT_EQ(pair_res.first, -5);
	ASSERT_EQ(pair_res.second, 0xfffffE6);
}

TEST(SerializeTest, ComplexPairSerializeDeserialize){
	std::unique_ptr<RMAInterface> rma{new MemoryRMA<20>()};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	TestPOD pod1{6, 444444, true, TestPOD::E::e2};
	SerializableImplTest si1{13, -2};
	std::pair<const std::pair<TestPOD, std::string>,
		std::pair<const uint64_t, SerializableImplTest>> pair1{{pod1, "t1anyways"}, {111, si1}};
	StorageAddress addr = serialize<StorageAddress>(*storage.get(), pair1);
	auto pair_res = deserialize<decltype(pair1)>(*storage.get(), addr);
	auto &pod_res = pair_res.first.first;
	auto &str_res = pair_res.first.second;
	auto &u64_res = pair_res.second.first;
	auto &si_res = pair_res.second.second;
	ASSERT_EQ(pod_res.a, pod1.a);
	ASSERT_EQ(pod_res.b, pod1.b);
	ASSERT_EQ(pod_res.c, pod1.c);
	ASSERT_EQ(pod_res.e, pod1.e);
	ASSERT_EQ(str_res, "t1anyways");
	ASSERT_EQ(u64_res, 111UL);
	ASSERT_EQ(si_res.m[13], -2);
}

TEST(SerializeTest, SimpleTupleSerializeDeserialize){
	std::unique_ptr<RMAInterface> rma{new MemoryRMA<20>()};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	std::tuple<int, uint64_t, const char> tuple1{-333, 0xeeef6, 'L'};
	StorageAddress addr = serialize<StorageAddress>(*storage.get(), tuple1);
	auto tuple_res = deserialize<decltype(tuple1)>(*storage.get(), addr);
	ASSERT_EQ(std::get<0>(tuple_res), -333);
	ASSERT_EQ(std::get<1>(tuple_res), 0xeeef6);
	ASSERT_EQ(std::get<2>(tuple_res), 'L');
}

TEST(SerializeTest, ComplexTupleSerializeDeserialize){
	std::unique_ptr<RMAInterface> rma{new MemoryRMA<20>()};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	SerializableImplTest si1{13, -2};
	TestPOD pod1{6, 444444, true, TestPOD::E::e2};
	std::tuple<SerializableImplTest, uint64_t,
		const std::pair<const char, TestPOD>> tuple1{si1, 0xeeef6, {'W', pod1}};
	StorageAddress addr = serialize<StorageAddress>(*storage.get(), tuple1);
	auto tuple_res = deserialize<decltype(tuple1)>(*storage.get(), addr);
	ASSERT_EQ(std::get<0>(tuple_res).m[13], -2);
	ASSERT_EQ(std::get<1>(tuple_res), 0xeeef6);
	ASSERT_EQ(std::get<2>(tuple_res).first, 'W');
	auto &pod_res = std::get<2>(tuple_res).second;
	ASSERT_EQ(pod_res.a, pod1.a);
	ASSERT_EQ(pod_res.b, pod1.b);
	ASSERT_EQ(pod_res.c, pod1.c);
	ASSERT_EQ(pod_res.e, pod1.e);
}

}
