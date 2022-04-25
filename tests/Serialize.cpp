
#include <Serialize.hpp>
#include <SimpleStorage.hpp>
#include <RandomMemoryAccess.hpp>

#include <cstdio>
#include <memory>

#include "gtest/gtest.h"


namespace{

class SerializableImplTest;

class SerializableImplTest:
    public ISerialize<SerializableImplTest>{
public:
    std::map<int, int> m;
    SerializableImplTest(int a, int b){
    	m[a] = b;
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

    virtual ~SerializableImplTest(){}
};

const std::string filename{"/tmp/FileRMA.test"};

TEST(SerializeTest, SimpleInit){
	::remove(filename.c_str());
	std::unique_ptr<RMAInterface> rma{new FileRMA<20>(filename)};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	SerializableImplTest si{1, 2};
	StorageAddress addr = serialize<StorageAddress>(*storage.get(), si);
	SerializableImplTest si2 = deserialize<SerializableImplTest>(*storage.get(), addr);
	ASSERT_EQ(si.m[1], si2.m[1]);
}

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
