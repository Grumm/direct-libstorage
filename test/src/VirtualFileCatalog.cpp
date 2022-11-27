
#include <cstdio>
#include <memory>
#include <storage/VirtualFileCatalog.hpp>

#include "gtest/gtest.h"

namespace {

static constexpr size_t MEMORYSIZE = 22;  // 4MB
static constexpr size_t FILESIZE = 22;  // 4MB
static const std::string filename1{"/tmp/.UniqueDataStoragePtrSerializeDeserialize1"};
static const std::string filename2{"/tmp/.UniqueDataStoragePtrSerializeDeserialize2"};

TEST(SimpleMultiLevelKeyMapTest, SimpleSerializeDeserialize) {
  SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};

  SimpleMultiLevelKeyMap<short, int, uint64_t, const char> smlkm1;
  smlkm1.add(-333, 0xeeef6, 'L', 13);
  smlkm1.add(555, 0xfffffffff16UL, 'T', -15000);
  smlkm1.add(1, 155566023013513, 's', 0);
  StorageAddress addr = serialize<StorageAddress>(storage, smlkm1);
  auto smlkm_res = deserialize<decltype(smlkm1)>(storage, addr);
  auto match1 = smlkm_res.get(-333, 0xeeef6, 'L');
  EXPECT_EQ(match1.first, true);
  EXPECT_EQ(match1.second, 13);
  EXPECT_EQ(smlkm_res.get(-333, 0xeeef6, 'W').first, false);
  auto match2 = smlkm_res.get(555, 0xfffffffff16UL, 'T');
  EXPECT_EQ(match2.first, true);
  EXPECT_EQ(match2.second, -15000);
  EXPECT_EQ(smlkm_res.get(555, 0, 'T').first, false);
  auto match3 = smlkm_res.get(1, 155566023013513, 's');
  EXPECT_EQ(match3.first, true);
  EXPECT_EQ(match3.second, 0);
  EXPECT_EQ(smlkm_res.get(0, 5, 'L').first, false);
}

using TestVFCType1 = VirtualFileCatalog<SimpleRamStorage<MEMORYSIZE>, int, char>;

TEST(VirtualFileCatalogTest, SimpleCreateLookup) {
  SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
  {
    TestVFCType1 _vfc1{storage};
    auto &vfc1 = _vfc1.get().get();
    EXPECT_EQ(vfc1.add('c', 3), Result::Success);
    EXPECT_EQ(vfc1.add('d', 2), Result::Success);
    EXPECT_EQ(vfc1.get('d'), std::make_pair(true, 2));
    EXPECT_EQ(vfc1.get('c'), std::make_pair(true, 3));
    EXPECT_EQ(vfc1.get('e').first, false);
  }
}

TEST(VirtualFileCatalogTest, SimpleSerializeDeserialize) {
  SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
  StorageAddress vfcaddr;
  {
    TestVFCType1 _vfc1{storage};
    auto &vfc1 = _vfc1.get().get();
    EXPECT_EQ(vfc1.add('c', 3), Result::Success);
    EXPECT_EQ(vfc1.add('d', 2), Result::Success);
    serialize(storage, _vfc1, vfcaddr);
  }
  {
    auto _vfc2 = deserialize<TestVFCType1>(storage, vfcaddr, storage, storage);
    auto &vfc2 = _vfc2.get().get();
    EXPECT_EQ(vfc2.get('d'), std::make_pair(true, 2));
    EXPECT_EQ(vfc2.get('c'), std::make_pair(true, 3));
    EXPECT_EQ(vfc2.get('e').first, false);
  }
}

TEST(VirtualFileCatalogTest, SimpleSerializeDeserializeAddTwice) {
  SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
  StorageAddress vfcaddr;
  {
    TestVFCType1 _vfc1{storage};
    auto &vfc1 = _vfc1.get().get();
    EXPECT_EQ(vfc1.add('c', 3), Result::Success);
    EXPECT_EQ(vfc1.add('d', 2), Result::Success);
    serialize(storage, _vfc1, vfcaddr);
  }
  {
    auto _vfc2 = deserialize<TestVFCType1>(storage, vfcaddr, storage, storage);
    auto &vfc2 = _vfc2.get().get();
    EXPECT_EQ(vfc2.get('d'), std::make_pair(true, 2));
    EXPECT_EQ(vfc2.get('c'), std::make_pair(true, 3));
    EXPECT_EQ(vfc2.get('e').first, false);
    EXPECT_EQ(vfc2.add('e', 7), Result::Success);
    EXPECT_EQ(vfc2.get('e'), std::make_pair(true, 7));
    serialize(storage, _vfc2, vfcaddr);
  }
  {
    auto _vfc3 = deserialize<TestVFCType1>(storage, vfcaddr, storage, storage);
    auto &vfc3 = _vfc3.get().get();
    EXPECT_EQ(vfc3.get('d'), std::make_pair(true, 2));
    EXPECT_EQ(vfc3.get('c'), std::make_pair(true, 3));
    EXPECT_EQ(vfc3.get('e'), std::make_pair(true, 7));
    EXPECT_EQ(vfc3.get('f').first, false);
  }
}

TEST(VirtualFileCatalogTest, SimpleSerializeDeserializeAddDeleteTwice) {
  SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
  StorageAddress vfcaddr;
  {
    TestVFCType1 _vfc1{storage};
    auto &vfc1 = _vfc1.get().get();
    EXPECT_EQ(vfc1.add('c', 3), Result::Success);
    EXPECT_EQ(vfc1.add('d', 2), Result::Success);
    serialize(storage, _vfc1, vfcaddr);
  }
  {
    auto _vfc2 = deserialize<TestVFCType1>(storage, vfcaddr, storage, storage);
    auto &vfc2 = _vfc2.get().get();
    EXPECT_EQ(vfc2.add('e', 7), Result::Success);
    vfc2.del('c');
    serialize(storage, _vfc2, vfcaddr);
  }
  {
    auto _vfc3 = deserialize<TestVFCType1>(storage, vfcaddr, storage, storage);
    auto &vfc3 = _vfc3.get().get();
    EXPECT_EQ(vfc3.get('d'), std::make_pair(true, 2));
    EXPECT_EQ(vfc3.get('c').first, false);
    EXPECT_EQ(vfc3.get('e'), std::make_pair(true, 7));
    EXPECT_EQ(vfc3.get('f').first, false);
  }
}

using UniqueDataStorage = UniqueIDPtr<DataStorageBase>;
using TestVFCType2 =
    VirtualFileCatalog<SimpleRamStorage<MEMORYSIZE>, UniqueDataStorage, int16_t, uint64_t>;
TEST(VirtualFileCatalogTest, UniqueDataStoragePtrSerializeDeserialize) {
  SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
  UniqueIDStorage<SimpleFileStorage<FILESIZE>, decltype(storage)> uid_s{storage};
  StorageAddress vfcaddr;
  {
    auto storage1 = new SimpleFileStorage<FILESIZE>{FileRMA<FILESIZE>{filename1}, 3};
    auto storage2 = new SimpleFileStorage<FILESIZE>{FileRMA<FILESIZE>{filename2}, 7};
    UniqueDataStorage uds1{storage1};
    UniqueDataStorage uds2{storage2};
  
    TestVFCType2 _vfc1{storage};
    auto &vfc1 = _vfc1.get().get();
    EXPECT_EQ(vfc1.add(15000, 0xfffffffffe, uds1), Result::Success);
    EXPECT_EQ(vfc1.add(-13, 114, uds2), Result::Success);
    serialize(storage, _vfc1, vfcaddr);
  }
  {
    auto _vfc2 = deserialize<TestVFCType2>(storage, vfcaddr, storage, storage);
    auto &vfc2 = _vfc2.get().get();
    auto r1 = vfc2.get(15000, 0xfffffffffe);
    auto r2 = vfc2.get(-13, 114);
    EXPECT_TRUE(r1.first);
    EXPECT_TRUE(r2.first);
    EXPECT_EQ(r1.second.getID(), 3);
    EXPECT_EQ(r2.second.getID(), 7);
  }
}

TEST(VirtualFileCatalogTest, UDSPtrSerializeDeserializeInstantiate) {
  SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
  UniqueIDStorage<SimpleFileStorage<FILESIZE>, decltype(storage)> uid_s{storage};
  StorageAddress addr1;
  StorageAddress addr2;
  StorageAddress vfcaddr;
  {
	  std::filesystem::remove(std::filesystem::path{filename1});
	  std::filesystem::remove(std::filesystem::path{filename2});
    auto storage1 = new SimpleFileStorage<FILESIZE>{FileRMA<FILESIZE>{filename1}, 3};
    auto storage2 = new SimpleFileStorage<FILESIZE>{FileRMA<FILESIZE>{filename2}, 7};
    UniqueDataStorage uds1{storage1};
    UniqueDataStorage uds2{storage2};

    addr1 = storage1->get_random_address(sizeof(int));
    addr2 = storage2->get_random_address(sizeof(int));
    auto buf1 = storage1->writeb(addr1);
    auto buf2 = storage2->writeb(addr2);
    buf1.get<int>()[0] = 33;
    buf2.get<int>()[0] = 14;
    storage1->commit(buf1);
    storage2->commit(buf2);

    uid_s.registerInstance(*storage1);
    uid_s.registerInstance(*storage2);

    TestVFCType2 _vfc1{storage};
    auto &vfc1 = _vfc1.get().get();
    EXPECT_EQ(vfc1.add(15000, 0xfffffffffe, uds1), Result::Success);
    EXPECT_EQ(vfc1.add(-13, 114, uds2), Result::Success);
    serialize(storage, _vfc1, vfcaddr);
  }
  {
    auto _vfc2 = deserialize<TestVFCType2>(storage, vfcaddr, storage, storage);
    auto &vfc2 = _vfc2.get().get();
    auto [has1, val1] = vfc2.get(15000, 0xfffffffffe);
    auto [has2, val2] = vfc2.get(-13, 114);
    EXPECT_TRUE(has1);
    EXPECT_TRUE(has2);
    EXPECT_EQ(val1.getID(), 3);
    EXPECT_EQ(val2.getID(), 7);
    EXPECT_FALSE(val1.is_init());
    EXPECT_FALSE(val2.is_init());
    val1.init(uid_s);
    val2.init(uid_s);
    EXPECT_TRUE(val1.is_init());
    EXPECT_TRUE(val2.is_init());
    auto storage1 = val1.getPtr<SimpleFileStorage<FILESIZE>>();
    auto storage2 = val2.getPtr<SimpleFileStorage<FILESIZE>>();
  
    EXPECT_EQ(val1.getID(), 3);
    EXPECT_EQ(val2.getID(), 7);
  
    auto buf1 = storage1->readb(addr1);
    auto buf2 = storage2->readb(addr2);
    EXPECT_EQ(buf1.get<int>()[0], 33);
    EXPECT_EQ(buf2.get<int>()[0], 14);
    storage1->commit(buf1);
    storage2->commit(buf2);
  }
}

TEST(VirtualFileCatalogTest, UDSPtrSDInstantiateSD) {
  SimpleRamStorage<MEMORYSIZE> storage{MemoryRMA<MEMORYSIZE>{}};
  StorageAddress addr1;
  StorageAddress addr2;
  StorageAddress addr_s1;
  StorageAddress addr_s2;
  StorageAddress addr_uids;
  StorageAddress vfcaddr;
  {
    UniqueIDStorage<SimpleFileStorage<FILESIZE>, decltype(storage)> uid_s{storage};
	  std::filesystem::remove(std::filesystem::path{filename1});
	  std::filesystem::remove(std::filesystem::path{filename2});
    auto storage1 = new SimpleFileStorage<FILESIZE>{FileRMA<FILESIZE>{filename1}, 3};
    auto storage2 = new SimpleFileStorage<FILESIZE>{FileRMA<FILESIZE>{filename2}, 7};
    UniqueDataStorage uds1{storage1};
    UniqueDataStorage uds2{storage2};

    addr1 = storage1->get_random_address(sizeof(int));
    addr2 = storage2->get_random_address(sizeof(int));
    auto buf1 = storage1->writeb(addr1);
    auto buf2 = storage2->writeb(addr2);
    buf1.get<int>()[0] = 33;
    buf2.get<int>()[0] = 14;
    storage1->commit(buf1);
    storage2->commit(buf2);

    addr_s1 = serialize<StorageAddress>(storage, *storage1);
    addr_s2 = serialize<StorageAddress>(storage, *storage2);
    uid_s.registerInstance(*storage1);
    uid_s.registerInstance(*storage2);
    uid_s.registerInstanceAddress(*storage1, addr_s1);
    uid_s.registerInstanceAddress(*storage2, addr_s2);

    addr_uids = serialize<StorageAddress>(storage, uid_s);

    TestVFCType2 _vfc1{storage};
    auto &vfc1 = _vfc1.get().get();
    EXPECT_EQ(vfc1.add(15000, 0xfffffffffe, uds1), Result::Success);
    EXPECT_EQ(vfc1.add(-13, 114, uds2), Result::Success);
    serialize(storage, _vfc1, vfcaddr);
  }
  {
    auto uid_s = deserialize<UniqueIDStorage<SimpleFileStorage<FILESIZE>, decltype(storage)>>(storage, addr_uids, storage);
    auto _vfc2 = deserialize<TestVFCType2>(storage, vfcaddr, storage, storage);
    auto &vfc2 = _vfc2.get().get();
    auto r1 = vfc2.get(15000, 0xfffffffffe);
    auto r2 = vfc2.get(-13, 114);
    EXPECT_TRUE(r1.first);
    EXPECT_TRUE(r2.first);
    EXPECT_EQ(r1.second.getID(), 3);
    EXPECT_EQ(r2.second.getID(), 7);
    EXPECT_FALSE(r1.second.is_init());
    EXPECT_FALSE(r2.second.is_init());
    r1.second.init(uid_s);
    r2.second.init(uid_s);
    EXPECT_TRUE(r1.second.is_init());
    EXPECT_TRUE(r2.second.is_init());
    auto storage1 = r1.second.getPtr<SimpleFileStorage<FILESIZE>>();
    auto storage2 = r2.second.getPtr<SimpleFileStorage<FILESIZE>>();
  
    EXPECT_EQ(r1.second.getID(), 3);
    EXPECT_EQ(r2.second.getID(), 7);
  
    auto buf1 = storage1->readb(addr1);
    auto buf2 = storage2->readb(addr2);
    EXPECT_EQ(buf1.get<int>()[0], 33);
    EXPECT_EQ(buf2.get<int>()[0], 14);
    storage1->commit(buf1);
    storage2->commit(buf2);
  }
}


}  // namespace