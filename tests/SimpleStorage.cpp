
#include <SimpleStorage.hpp>
#include <cstdio>
#include <memory>

#include "gtest/gtest.h"

namespace{

const std::string filename{"/tmp/FileRMA.test"};

TEST(SimpleStorage, ReadWriteSimple){
	::remove(filename.c_str());
	std::unique_ptr<RMAInterface> rma{new FileRMA<20>(filename)};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	auto address = storage->get_random_address(256);
	EXPECT_NE(address.addr, (uint64_t)0UL);
	EXPECT_NE(address.size, 0UL);

	std::unique_ptr<unsigned char[]> mem1{new unsigned char[256]};
	std::unique_ptr<unsigned char[]> mem2{new unsigned char[256]};
	StorageBuffer buf1{mem1.get(), 100, 256};
	StorageBuffer buf2{mem2.get(), 0, 256};

	for(size_t i = 0; i < 100; i++){
		mem1[i] = i;
		mem2[i] = 0;
	}
	EXPECT_EQ(Result::Success, storage->write(address, buf1));
	EXPECT_EQ(Result::Success, storage->read(address, buf2));
	for(size_t i = 0; i < 100; i++){
		EXPECT_EQ(mem2[i], i);
	}

	StorageBufferRO readb = storage->readb(address);
	for(size_t i = 0; i < 100; i++){
		EXPECT_EQ(((const unsigned char *)readb.data)[i], i);
	}
	EXPECT_EQ(Result::Success, storage->commit(readb));
}

TEST(SimpleStorage, ReadWriteSimpleBuffer){
	::remove(filename.c_str());
	std::unique_ptr<RMAInterface> rma{new FileRMA<20>(filename)};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	auto address = storage->get_random_address(256);
	EXPECT_NE(address.addr, (uint64_t)0UL);
	EXPECT_NE(address.size, 0UL);

	StorageBuffer writeb = storage->writeb(address);
	for(size_t i = 0; i < 100; i++){
		((unsigned char *)writeb.data)[i] = i;
	}
	EXPECT_EQ(Result::Success, storage->commit(writeb));

	StorageBufferRO readb = storage->readb(address);
	for(size_t i = 0; i < 100; i++){
		EXPECT_EQ(((const unsigned char *)readb.data)[i], i);
	}
	EXPECT_EQ(Result::Success, storage->commit(readb));
}

TEST(SimpleStorage, ReadIncorrectAddress){
	::remove(filename.c_str());
	std::unique_ptr<RMAInterface> rma{new FileRMA<20>(filename)};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	auto address = storage->get_random_address(256);
	EXPECT_NE(address.addr, (uint64_t)0UL);
	EXPECT_NE(address.size, 0UL);

	StorageBuffer writeb = storage->writeb(address);
	for(size_t i = 0; i < 100; i++){
		((unsigned char *)writeb.data)[i] = i;
	}
	EXPECT_EQ(Result::Success, storage->commit(writeb));

	EXPECT_THROW(storage->readb(StorageAddress{500, 100}), std::logic_error);
	EXPECT_THROW(storage->readb(StorageAddress{888, 50}), std::logic_error);
}

TEST(SimpleStorage, ReadWriteMultipleBuffers){
	::remove(filename.c_str());
	std::unique_ptr<RMAInterface> rma{new FileRMA<20>(filename)};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	auto address1 = storage->get_random_address(100);
	EXPECT_NE(address1.addr, (uint64_t)0UL);
	EXPECT_NE(address1.size, 0UL);
	auto address2 = storage->get_random_address(50);
	EXPECT_NE(address2.addr, (uint64_t)0UL);
	EXPECT_NE(address2.size, 0UL);
	auto address3 = storage->get_random_address(50);
	EXPECT_NE(address3.addr, (uint64_t)0UL);
	EXPECT_NE(address3.size, 0UL);

	StorageBuffer writeb1 = storage->writeb(address1);
	for(size_t i = 0; i < 100; i++){
		((unsigned char *)writeb1.data)[i] = i;
	}
	EXPECT_EQ(Result::Success, storage->commit(writeb1));
	StorageBuffer writeb2 = storage->writeb(address2);
	for(size_t i = 0; i < 50; i++){
		((unsigned char *)writeb2.data)[i] = 100 + i;
	}
	EXPECT_EQ(Result::Success, storage->commit(writeb2));
	StorageBuffer writeb3 = storage->writeb(address3);
	for(size_t i = 0; i < 50; i++){
		((unsigned char *)writeb3.data)[i] = 150 + i;
	}
	EXPECT_EQ(Result::Success, storage->commit(writeb3));

	StorageBufferRO readb1 = storage->readb(address1);
	for(size_t i = 0; i < 100; i++){
		EXPECT_EQ(((const unsigned char *)readb1.data)[i], i);
	}
	EXPECT_EQ(Result::Success, storage->commit(readb1));
	StorageBufferRO readb2 = storage->readb(address2);
	for(size_t i = 0; i < 50; i++){
		EXPECT_EQ(((const unsigned char *)readb2.data)[i], 100 + i);
	}
	EXPECT_EQ(Result::Success, storage->commit(readb2));
	StorageBufferRO readb3 = storage->readb(address3);
	for(size_t i = 0; i < 50; i++){
		EXPECT_EQ(((const unsigned char *)readb3.data)[i], 150 + i);
	}
	EXPECT_EQ(Result::Success, storage->commit(readb3));
}

TEST(SimpleStorage, ReadWriteEraseSimple){
	::remove(filename.c_str());
	std::unique_ptr<RMAInterface> rma{new FileRMA<20>(filename)};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	auto address = storage->get_random_address(256);
	EXPECT_NE(address.addr, (uint64_t)0UL);
	EXPECT_NE(address.size, 0UL);

	StorageBuffer writeb = storage->writeb(address);
	for(size_t i = 0; i < 100; i++){
		((unsigned char *)writeb.data)[i] = i;
	}
	EXPECT_EQ(Result::Success, storage->commit(writeb));
	EXPECT_EQ(Result::Success, storage->erase(address));
	EXPECT_EQ(Result::Failure, storage->erase(address));

	EXPECT_THROW(storage->readb(address), std::logic_error);
}

TEST(SimpleStorage, ReadWriteEraseMultipleAddresses){
	::remove(filename.c_str());
	std::unique_ptr<RMAInterface> rma{new FileRMA<20>(filename)};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	auto address1 = storage->get_random_address(100);
	EXPECT_NE(address1.addr, (uint64_t)0UL);
	EXPECT_NE(address1.size, 0UL);
	auto address2 = storage->get_random_address(50);
	EXPECT_NE(address2.addr, (uint64_t)0UL);
	EXPECT_NE(address2.size, 0UL);
	auto address3 = storage->get_random_address(50);
	EXPECT_NE(address3.addr, (uint64_t)0UL);
	EXPECT_NE(address3.size, 0UL);

	StorageBuffer writeb1 = storage->writeb(address1);
	for(size_t i = 0; i < 100; i++){
		((unsigned char *)writeb1.data)[i] = i;
	}
	EXPECT_EQ(Result::Success, storage->commit(writeb1));
	StorageBuffer writeb2 = storage->writeb(address2);
	for(size_t i = 0; i < 50; i++){
		((unsigned char *)writeb2.data)[i] = 100 + i;
	}
	EXPECT_EQ(Result::Success, storage->commit(writeb2));
	EXPECT_EQ(Result::Success, storage->erase(address2));
	EXPECT_THROW(storage->readb(address2), std::logic_error);

	StorageBuffer writeb3 = storage->writeb(address3);
	for(size_t i = 0; i < 50; i++){
		((unsigned char *)writeb3.data)[i] = 150 + i;
	}
	EXPECT_EQ(Result::Success, storage->commit(writeb3));

	StorageBufferRO readb1 = storage->readb(address1);
	for(size_t i = 0; i < 100; i++){
		EXPECT_EQ(((const unsigned char *)readb1.data)[i], i);
	}
	EXPECT_EQ(Result::Success, storage->commit(readb1));
	StorageBufferRO readb3 = storage->readb(address3);
	for(size_t i = 0; i < 50; i++){
		EXPECT_EQ(((const unsigned char *)readb3.data)[i], 150 + i);
	}
	EXPECT_EQ(Result::Success, storage->commit(readb3));

	EXPECT_THROW(storage->readb(address2), std::logic_error);
	EXPECT_EQ(Result::Success, storage->erase(address3));
	EXPECT_THROW(storage->readb(address3), std::logic_error);
	EXPECT_EQ(Result::Success, storage->erase(address1));
	EXPECT_THROW(storage->readb(address1), std::logic_error);
	EXPECT_EQ(Result::Failure, storage->erase(address2));
}

TEST(SimpleStorage, ReadWriteEraseMultipleAddressesOverwrite){
	::remove(filename.c_str());
	std::unique_ptr<RMAInterface> rma{new FileRMA<20>(filename)};
	std::unique_ptr<DataStorage> storage{new SimpleStorage{*rma.get()}};

	auto address1 = storage->get_random_address(100);
	EXPECT_NE(address1.addr, (uint64_t)0UL);
	EXPECT_NE(address1.size, 0UL);
	auto address2 = storage->get_random_address(50);
	EXPECT_NE(address2.addr, (uint64_t)0UL);
	EXPECT_NE(address2.size, 0UL);
	auto address3 = storage->get_random_address(50);
	EXPECT_NE(address3.addr, (uint64_t)0UL);
	EXPECT_NE(address3.size, 0UL);

	StorageBuffer writeb1 = storage->writeb(address1);
	for(size_t i = 0; i < 100; i++){
		((unsigned char *)writeb1.data)[i] = i;
	}
	EXPECT_EQ(Result::Success, storage->commit(writeb1));
	StorageBuffer writeb2 = storage->writeb(address2);
	for(size_t i = 0; i < 50; i++){
		((unsigned char *)writeb2.data)[i] = 100 + i;
	}
	EXPECT_EQ(Result::Success, storage->commit(writeb2));

	EXPECT_EQ(Result::Success, storage->erase(address2));
	EXPECT_THROW(storage->readb(address2), std::logic_error);

	StorageBuffer writeb3 = storage->writeb(address3);
	for(size_t i = 0; i < 50; i++){
		((unsigned char *)writeb3.data)[i] = 150 + i;
	}
	EXPECT_EQ(Result::Success, storage->commit(writeb3));
	writeb2 = storage->writeb(address2);
	for(size_t i = 0; i < 50; i++){
		((unsigned char *)writeb2.data)[i] = 100 + i;
	}
	EXPECT_EQ(Result::Success, storage->commit(writeb2));

	StorageBufferRO readb1 = storage->readb(address1);
	for(size_t i = 0; i < 100; i++){
		EXPECT_EQ(((const unsigned char *)readb1.data)[i], i);
	}
	EXPECT_EQ(Result::Success, storage->commit(readb1));
	StorageBufferRO readb2 = storage->readb(address2);
	for(size_t i = 0; i < 50; i++){
		EXPECT_EQ(((const unsigned char *)readb2.data)[i], 100 + i);
	}
	EXPECT_EQ(Result::Success, storage->commit(readb2));
	StorageBufferRO readb3 = storage->readb(address3);
	for(size_t i = 0; i < 50; i++){
		EXPECT_EQ(((const unsigned char *)readb3.data)[i], 150 + i);
	}
	EXPECT_EQ(Result::Success, storage->commit(readb3));
}

//TODO rw addr+offset, read with size_incomplete
//TODO write incorrect
//TODO expand address
//TODO serialize

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
