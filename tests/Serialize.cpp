
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

    Result serializeImpl(const StorageBuffer &buffer) const {
    	int *i = static_cast<int *>(buffer.data);
    	i[0] = m.begin()->first;
    	i[1] = m.begin()->second;
    	return Result::Success;
    }
    static SerializableImplTest deserializeImpl(const StorageBufferRO &buffer) {
    	const int *i = static_cast<const int *>(buffer.data);
    	return SerializableImplTest{i[0], i[1]};
    }
    size_t getSizeImpl() const {
    	return sizeof(int) * 2;
    }
};

const std::string filename{"/tmp/FileRMA.test"};

TEST(SerializeTest, SimpleSerializeDeserialize){
	::remove(filename.c_str());
	std::unique_ptr<RMAInterface> rma{new FileRMA<20>(filename)};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	SerializableImplTest si{1, 2};
	StorageAddress addr = serialize<StorageAddress>(*storage.get(), si);
	SerializableImplTest si_res = deserialize<SerializableImplTest>(*storage.get(), addr);
	ASSERT_EQ(si.m[1], si_res.m[1]);
}

TEST(SerializeTest, SimpleOverwrite){
	::remove(filename.c_str());
	std::unique_ptr<RMAInterface> rma{new FileRMA<20>(filename)};
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
	::remove(filename.c_str());
	std::unique_ptr<RMAInterface> rma{new FileRMA<20>(filename)};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	int a = 3;
	StorageAddress addr = serialize<StorageAddress>(*storage.get(), a);
	int a_res = deserialize<int>(*storage.get(), addr);
	ASSERT_EQ(a, a_res);
}

TEST(SerializeTest, IntegralOverwrite){
	::remove(filename.c_str());
	std::unique_ptr<RMAInterface> rma{new FileRMA<20>(filename)};
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
	::remove(filename.c_str());
	std::unique_ptr<RMAInterface> rma{new FileRMA<20>(filename)};
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
	::remove(filename.c_str());
	std::unique_ptr<RMAInterface> rma{new FileRMA<20>(filename)};
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
	::remove(filename.c_str());
	std::unique_ptr<RMAInterface> rma{new FileRMA<20>(filename)};
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
	::remove(filename.c_str());
	std::unique_ptr<RMAInterface> rma{new FileRMA<20>(filename)};
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

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
