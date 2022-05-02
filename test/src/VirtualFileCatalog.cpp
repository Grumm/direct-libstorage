
#include <cstdio>
#include <memory>

#include <storage/VirtualFileCatalog.hpp>

#include "gtest/gtest.h"


namespace{

TEST(SimpleMultiLevelKeyMapTest, SimpleSerializeDeserialize){
	std::unique_ptr<RMAInterface> rma{new MemoryRMA<20>()};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	SimpleMultiLevelKeyMap<short, int, uint64_t, const char> smlkm1;
    smlkm1.add(-333, 0xeeef6, 'L', 13);
	StorageAddress addr = serialize<StorageAddress>(*storage.get(), smlkm1);
	auto smlkm_res = deserialize<decltype(smlkm1)>(*storage.get(), addr);
    auto match1 = smlkm_res.get(-333, 0xeeef6, 'L');
	ASSERT_EQ(match1.first, true);
	ASSERT_EQ(match1.second, 13);
	ASSERT_EQ(smlkm_res.get(-333, 0xeeef6, 'W').first, false);
}

}