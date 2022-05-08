
#include <cstdio>
#include <memory>

#include <storage/VirtualFileCatalog.hpp>

#include "gtest/gtest.h"


namespace{

static constexpr size_t MEMORYSIZE = 20; // 1MB

TEST(SimpleMultiLevelKeyMapTest, SimpleSerializeDeserialize){
    auto rma = std::make_unique<MemoryRMA<MEMORYSIZE>>();
    auto storage = std::make_unique<SimpleRamStorage<MEMORYSIZE>>(*rma.get());

    SimpleMultiLevelKeyMap<short, int, uint64_t, const char> smlkm1;
    smlkm1.add(-333, 0xeeef6, 'L', 13);
    smlkm1.add(555, 0xfffffffff16UL, 'T', -15000);
    smlkm1.add(1, 155566023013513, 's', 0);
    StorageAddress addr = serialize<StorageAddress>(*storage.get(), smlkm1);
    auto smlkm_res = deserialize<decltype(smlkm1)>(*storage.get(), addr);
    auto match1 = smlkm_res.get(-333, 0xeeef6, 'L');
    ASSERT_EQ(match1.first, true);
    ASSERT_EQ(match1.second, 13);
    ASSERT_EQ(smlkm_res.get(-333, 0xeeef6, 'W').first, false);
    auto match2 = smlkm_res.get(555, 0xfffffffff16UL, 'T');
    ASSERT_EQ(match2.first, true);
    ASSERT_EQ(match2.second, -15000);
    ASSERT_EQ(smlkm_res.get(555, 0, 'T').first, false);
    auto match3 = smlkm_res.get(1, 155566023013513, 's');
    ASSERT_EQ(match3.first, true);
    ASSERT_EQ(match3.second, 0);
    ASSERT_EQ(smlkm_res.get(0, 5, 'L').first, false);
}

}