
#include <cstdio>
#include <memory>
#include <filesystem>

#include <storage/SimpleStorage.hpp>

#include "gtest/gtest.h"

namespace{

const std::string filename{"/tmp/FileRMA.test"};
constexpr size_t FILESIZE = 12;

TEST(SimpleStorage, ReadWriteSimple){
	std::filesystem::remove(std::filesystem::path{filename});
  	SimpleFileStorage<FILESIZE> storage{FileRMA<FILESIZE>{filename}};

	auto address = storage.get_random_address(256);
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
	EXPECT_EQ(Result::Success, storage.write(address, buf1));
	EXPECT_EQ(Result::Success, storage.read(address, buf2));
	for(size_t i = 0; i < 100; i++){
		EXPECT_EQ(mem2[i], i);
	}

	StorageBufferRO readb = storage.readb(address);
	for(size_t i = 0; i < 100; i++){
		EXPECT_EQ(readb.get<unsigned char>()[i], i);
	}
	EXPECT_EQ(Result::Success, storage.commit(readb));
}

TEST(SimpleStorage, ReadWriteSimpleBuffer){
	std::filesystem::remove(std::filesystem::path{filename});
  	SimpleFileStorage<FILESIZE> storage{FileRMA<FILESIZE>{filename}};

	auto address = storage.get_random_address(256);
	EXPECT_NE(address.addr, (uint64_t)0UL);
	EXPECT_NE(address.size, 0UL);

	StorageBuffer writeb = storage.writeb(address);
	for(size_t i = 0; i < 100; i++){
		writeb.get<unsigned char>()[i] = i;
	}
	EXPECT_EQ(Result::Success, storage.commit(writeb));

	StorageBufferRO readb = storage.readb(address);
	for(size_t i = 0; i < 100; i++){
		EXPECT_EQ(readb.get<unsigned char>()[i], i);
	}
	EXPECT_EQ(Result::Success, storage.commit(readb));
}

TEST(SimpleStorage, ReadIncorrectAddress){
	std::filesystem::remove(std::filesystem::path{filename});
  	SimpleFileStorage<FILESIZE> storage{FileRMA<FILESIZE>{filename}};

	auto address = storage.get_random_address(256);
	EXPECT_NE(address.addr, (uint64_t)0UL);
	EXPECT_NE(address.size, 0UL);

	StorageBuffer writeb = storage.writeb(address);
	for(size_t i = 0; i < 100; i++){
		writeb.get<unsigned char>()[i] = i;
	}
	EXPECT_EQ(Result::Success, storage.commit(writeb));

	EXPECT_THROW(storage.readb(StorageAddress{500, 100}), std::logic_error);
	EXPECT_THROW(storage.readb(StorageAddress{888, 50}), std::logic_error);
}

TEST(SimpleStorage, ReadWriteMultipleBuffers){
	std::filesystem::remove(std::filesystem::path{filename});
  	SimpleFileStorage<FILESIZE> storage{FileRMA<FILESIZE>{filename}};

	auto address1 = storage.get_random_address(100);
	EXPECT_NE(address1.addr, (uint64_t)0UL);
	EXPECT_NE(address1.size, 0UL);
	auto address2 = storage.get_random_address(50);
	EXPECT_NE(address2.addr, (uint64_t)0UL);
	EXPECT_NE(address2.size, 0UL);
	auto address3 = storage.get_random_address(50);
	EXPECT_NE(address3.addr, (uint64_t)0UL);
	EXPECT_NE(address3.size, 0UL);

	StorageBuffer writeb1 = storage.writeb(address1);
	for(size_t i = 0; i < 100; i++){
		writeb1.get<unsigned char>()[i] = i;
	}
	EXPECT_EQ(Result::Success, storage.commit(writeb1));
	StorageBuffer writeb2 = storage.writeb(address2);
	for(size_t i = 0; i < 50; i++){
		writeb2.get<unsigned char>()[i] = 100 + i;
	}
	EXPECT_EQ(Result::Success, storage.commit(writeb2));
	StorageBuffer writeb3 = storage.writeb(address3);
	for(size_t i = 0; i < 50; i++){
		writeb3.get<unsigned char>()[i] = 150 + i;
	}
	EXPECT_EQ(Result::Success, storage.commit(writeb3));

	StorageBufferRO readb1 = storage.readb(address1);
	for(size_t i = 0; i < 100; i++){
		EXPECT_EQ(readb1.get<unsigned char>()[i], i);
	}
	EXPECT_EQ(Result::Success, storage.commit(readb1));
	StorageBufferRO readb2 = storage.readb(address2);
	for(size_t i = 0; i < 50; i++){
		EXPECT_EQ(readb2.get<unsigned char>()[i], 100 + i);
	}
	EXPECT_EQ(Result::Success, storage.commit(readb2));
	StorageBufferRO readb3 = storage.readb(address3);
	for(size_t i = 0; i < 50; i++){
		EXPECT_EQ(readb3.get<unsigned char>()[i], 150 + i);
	}
	EXPECT_EQ(Result::Success, storage.commit(readb3));
}

TEST(SimpleStorage, ReadWriteEraseSimple){
	std::filesystem::remove(std::filesystem::path{filename});
  	SimpleFileStorage<FILESIZE> storage{FileRMA<FILESIZE>{filename}};

	auto address = storage.get_random_address(256);
	EXPECT_NE(address.addr, (uint64_t)0UL);
	EXPECT_NE(address.size, 0UL);

	StorageBuffer writeb = storage.writeb(address);
	for(size_t i = 0; i < 100; i++){
		writeb.get<unsigned char>()[i] = i;
	}
	EXPECT_EQ(Result::Success, storage.commit(writeb));
	EXPECT_EQ(Result::Success, storage.erase(address));
	EXPECT_EQ(Result::Failure, storage.erase(address));

	EXPECT_THROW(storage.readb(address), std::logic_error);
}

TEST(SimpleStorage, ReadWriteEraseMultipleAddresses){
	std::filesystem::remove(std::filesystem::path{filename});
  	SimpleFileStorage<FILESIZE> storage{FileRMA<FILESIZE>{filename}};

	auto address1 = storage.get_random_address(100);
	EXPECT_NE(address1.addr, (uint64_t)0UL);
	EXPECT_NE(address1.size, 0UL);
	auto address2 = storage.get_random_address(50);
	EXPECT_NE(address2.addr, (uint64_t)0UL);
	EXPECT_NE(address2.size, 0UL);
	auto address3 = storage.get_random_address(50);
	EXPECT_NE(address3.addr, (uint64_t)0UL);
	EXPECT_NE(address3.size, 0UL);

	StorageBuffer writeb1 = storage.writeb(address1);
	for(size_t i = 0; i < 100; i++){
		writeb1.get<unsigned char>()[i] = i;
	}
	EXPECT_EQ(Result::Success, storage.commit(writeb1));
	StorageBuffer writeb2 = storage.writeb(address2);
	for(size_t i = 0; i < 50; i++){
		writeb2.get<unsigned char>()[i] = 100 + i;
	}
	EXPECT_EQ(Result::Success, storage.commit(writeb2));
	EXPECT_EQ(Result::Success, storage.erase(address2));
	EXPECT_THROW(storage.readb(address2), std::logic_error);

	StorageBuffer writeb3 = storage.writeb(address3);
	for(size_t i = 0; i < 50; i++){
		writeb3.get<unsigned char>()[i] = 150 + i;
	}
	EXPECT_EQ(Result::Success, storage.commit(writeb3));

	StorageBufferRO readb1 = storage.readb(address1);
	for(size_t i = 0; i < 100; i++){
		EXPECT_EQ(readb1.get<unsigned char>()[i], i);
	}
	EXPECT_EQ(Result::Success, storage.commit(readb1));
	StorageBufferRO readb3 = storage.readb(address3);
	for(size_t i = 0; i < 50; i++){
		EXPECT_EQ(readb3.get<unsigned char>()[i], 150 + i);
	}
	EXPECT_EQ(Result::Success, storage.commit(readb3));

	EXPECT_THROW(storage.readb(address2), std::logic_error);
	EXPECT_EQ(Result::Success, storage.erase(address3));
	EXPECT_THROW(storage.readb(address3), std::logic_error);
	EXPECT_EQ(Result::Success, storage.erase(address1));
	EXPECT_THROW(storage.readb(address1), std::logic_error);
	EXPECT_EQ(Result::Failure, storage.erase(address2));
}

TEST(SimpleStorage, ReadWriteEraseMultipleAddressesOverwrite){
	std::filesystem::remove(std::filesystem::path{filename});
  	SimpleFileStorage<FILESIZE> storage{FileRMA<FILESIZE>{filename}};

	auto address1 = storage.get_random_address(100);
	EXPECT_NE(address1.addr, (uint64_t)0UL);
	EXPECT_NE(address1.size, 0UL);
	auto address2 = storage.get_random_address(50);
	EXPECT_NE(address2.addr, (uint64_t)0UL);
	EXPECT_NE(address2.size, 0UL);
	auto address3 = storage.get_random_address(50);
	EXPECT_NE(address3.addr, (uint64_t)0UL);
	EXPECT_NE(address3.size, 0UL);

	StorageBuffer writeb1 = storage.writeb(address1);
	for(size_t i = 0; i < 100; i++){
		writeb1.get<unsigned char>()[i] = i;
	}
	EXPECT_EQ(Result::Success, storage.commit(writeb1));
	StorageBuffer writeb2 = storage.writeb(address2);
	for(size_t i = 0; i < 50; i++){
		writeb2.get<unsigned char>()[i] = 100 + i;
	}
	EXPECT_EQ(Result::Success, storage.commit(writeb2));

	EXPECT_EQ(Result::Success, storage.erase(address2));
	EXPECT_THROW(storage.readb(address2), std::logic_error);

	StorageBuffer writeb3 = storage.writeb(address3);
	for(size_t i = 0; i < 50; i++){
		writeb3.get<unsigned char>()[i] = 150 + i;
	}
	EXPECT_EQ(Result::Success, storage.commit(writeb3));
	writeb2 = storage.writeb(address2);
	for(size_t i = 0; i < 50; i++){
		writeb2.get<unsigned char>()[i] = 100 + i;
	}
	EXPECT_EQ(Result::Success, storage.commit(writeb2));

	StorageBufferRO readb1 = storage.readb(address1);
	for(size_t i = 0; i < 100; i++){
		EXPECT_EQ(readb1.get<unsigned char>()[i], i);
	}
	EXPECT_EQ(Result::Success, storage.commit(readb1));
	StorageBufferRO readb2 = storage.readb(address2);
	for(size_t i = 0; i < 50; i++){
		EXPECT_EQ(readb2.get<unsigned char>()[i], 100 + i);
	}
	EXPECT_EQ(Result::Success, storage.commit(readb2));
	StorageBufferRO readb3 = storage.readb(address3);
	for(size_t i = 0; i < 50; i++){
		EXPECT_EQ(readb3.get<unsigned char>()[i], 150 + i);
	}
	EXPECT_EQ(Result::Success, storage.commit(readb3));
}

TEST(SimpleStorage, SerializeDeserializeSimple){
	std::filesystem::remove(std::filesystem::path{filename});

	StorageAddress address1;
	{
  		SimpleFileStorage<FILESIZE> storage{FileRMA<FILESIZE>{filename}};

		address1 = storage.get_random_address(100);
		EXPECT_NE(address1.addr, (uint64_t)0UL);
		EXPECT_NE(address1.size, 0UL);

		StorageBuffer writeb1 = storage.writeb(address1);
		for(size_t i = 0; i < 100; i++){
			writeb1.get<unsigned char>()[i] = i;
		}
		EXPECT_EQ(Result::Success, storage.commit(writeb1));

		StorageBufferRO readb1 = storage.readb(address1);
		for(size_t i = 0; i < 100; i++){
			EXPECT_EQ(readb1.get<unsigned char>()[i], i);
		}
		EXPECT_EQ(Result::Success, storage.commit(readb1));
	}
	{
  		SimpleFileStorage<FILESIZE> storage{FileRMA<FILESIZE>{filename}};

		StorageBufferRO readb1 = storage.readb(address1);
		for(size_t i = 0; i < 100; i++){
			EXPECT_EQ(readb1.get<unsigned char>()[i], i);
		}
		EXPECT_EQ(Result::Success, storage.commit(readb1));
	}
}

TEST(SimpleStorage, SerializeDeserializeMultipleAddresses){
	std::filesystem::remove(std::filesystem::path{filename});

	StorageAddress address1, address2, address3;
	{
  		SimpleFileStorage<FILESIZE> storage{FileRMA<FILESIZE>{filename}};

		address1 = storage.get_random_address(100);
		EXPECT_NE(address1.addr, (uint64_t)0UL);
		EXPECT_NE(address1.size, 0UL);
		address2 = storage.get_random_address(50);
		EXPECT_NE(address2.addr, (uint64_t)0UL);
		EXPECT_NE(address2.size, 0UL);
		address3 = storage.get_random_address(50);
		EXPECT_NE(address3.addr, (uint64_t)0UL);
		EXPECT_NE(address3.size, 0UL);

		StorageBuffer writeb1 = storage.writeb(address1);
		for(size_t i = 0; i < 100; i++){
			writeb1.get<unsigned char>()[i] = i;
		}
		EXPECT_EQ(Result::Success, storage.commit(writeb1));
		StorageBuffer writeb2 = storage.writeb(address2);
		for(size_t i = 0; i < 50; i++){
			writeb2.get<unsigned char>()[i] = 100 + i;
		}
		EXPECT_EQ(Result::Success, storage.commit(writeb2));

		StorageBuffer writeb3 = storage.writeb(address3);
		for(size_t i = 0; i < 50; i++){
			writeb3.get<unsigned char>()[i] = 150 + i;
		}
		EXPECT_EQ(Result::Success, storage.commit(writeb3));

		EXPECT_EQ(Result::Success, storage.erase(address2));
		EXPECT_THROW(storage.readb(address2), std::logic_error);

		StorageBufferRO readb1 = storage.readb(address1);
		for(size_t i = 0; i < 100; i++){
			EXPECT_EQ(readb1.get<unsigned char>()[i], i);
		}
		EXPECT_EQ(Result::Success, storage.commit(readb1));

		StorageBufferRO readb3 = storage.readb(address3);
		for(size_t i = 0; i < 50; i++){
			EXPECT_EQ(readb3.get<unsigned char>()[i], 150 + i);
		}
		EXPECT_EQ(Result::Success, storage.commit(readb3));
	}
	{
  		SimpleFileStorage<FILESIZE> storage{FileRMA<FILESIZE>{filename}};

		EXPECT_THROW(storage.readb(address2), std::logic_error);

		StorageBufferRO readb1 = storage.readb(address1);
		for(size_t i = 0; i < 100; i++){
			EXPECT_EQ(readb1.get<unsigned char>()[i], i);
		}
		EXPECT_EQ(Result::Success, storage.commit(readb1));

		StorageBufferRO readb3 = storage.readb(address3);
		for(size_t i = 0; i < 50; i++){
			EXPECT_EQ(readb3.get<unsigned char>()[i], 150 + i);
		}
		EXPECT_EQ(Result::Success, storage.commit(readb3));
	}
}

TEST(SimpleStorage, SerializeDeserializeMultipleAddressesTwoTimes){
	std::filesystem::remove(std::filesystem::path{filename});

	StorageAddress address1, address2, address3, address4, address5;
	{
  		SimpleFileStorage<FILESIZE> storage{FileRMA<FILESIZE>{filename}};

		address1 = storage.get_random_address(100);
		EXPECT_NE(address1.addr, (uint64_t)0UL);
		EXPECT_NE(address1.size, 0UL);
		address2 = storage.get_random_address(50);
		EXPECT_NE(address2.addr, (uint64_t)0UL);
		EXPECT_NE(address2.size, 0UL);
		address3 = storage.get_random_address(50);
		EXPECT_NE(address3.addr, (uint64_t)0UL);
		EXPECT_NE(address3.size, 0UL);

		StorageBuffer writeb1 = storage.writeb(address1);
		for(size_t i = 0; i < 100; i++){
			writeb1.get<unsigned char>()[i] = i;
		}
		EXPECT_EQ(Result::Success, storage.commit(writeb1));
		StorageBuffer writeb2 = storage.writeb(address2);
		for(size_t i = 0; i < 50; i++){
			writeb2.get<unsigned char>()[i] = 100 + i;
		}
		EXPECT_EQ(Result::Success, storage.commit(writeb2));

		StorageBuffer writeb3 = storage.writeb(address3);
		for(size_t i = 0; i < 50; i++){
			writeb3.get<unsigned char>()[i] = 150 + i;
		}
		EXPECT_EQ(Result::Success, storage.commit(writeb3));

		EXPECT_EQ(Result::Success, storage.erase(address2));
		EXPECT_THROW(storage.readb(address2), std::logic_error);

		StorageBufferRO readb1 = storage.readb(address1);
		for(size_t i = 0; i < 100; i++){
			EXPECT_EQ(readb1.get<unsigned char>()[i], i);
		}
		EXPECT_EQ(Result::Success, storage.commit(readb1));

		StorageBufferRO readb3 = storage.readb(address3);
		for(size_t i = 0; i < 50; i++){
			EXPECT_EQ(readb3.get<unsigned char>()[i], 150 + i);
		}
		EXPECT_EQ(Result::Success, storage.commit(readb3));
	}
	{
  		SimpleFileStorage<FILESIZE> storage{FileRMA<FILESIZE>{filename}};

		EXPECT_THROW(storage.readb(address2), std::logic_error);

		address4 = storage.get_random_address(60);
		EXPECT_NE(address4.addr, (uint64_t)0UL);
		EXPECT_NE(address4.size, 0UL);
		address5 = storage.get_random_address(24);
		EXPECT_NE(address5.addr, (uint64_t)0UL);
		EXPECT_NE(address5.size, 0UL);

		StorageBuffer writeb4 = storage.writeb(address4);
		for(size_t i = 0; i < 60; i++){
			writeb4.get<unsigned char>()[i] = i;
		}
		EXPECT_EQ(Result::Success, storage.commit(writeb4));
		StorageBuffer writeb5 = storage.writeb(address5);
		for(size_t i = 0; i < 24; i++){
			writeb5.get<unsigned char>()[i] = 60 + i;
		}
		EXPECT_EQ(Result::Success, storage.commit(writeb5));

		StorageBuffer writeb2 = storage.writeb(address2);
		for(size_t i = 0; i < address2.size; i++){
			writeb2.get<unsigned char>()[i] = 99 + i;
		}
		EXPECT_EQ(Result::Success, storage.commit(writeb2));

		StorageBufferRO readb1 = storage.readb(address1);
		for(size_t i = 0; i < 100; i++){
			EXPECT_EQ(readb1.get<unsigned char>()[i], i);
		}
		EXPECT_EQ(Result::Success, storage.commit(readb1));

		StorageBufferRO readb3 = storage.readb(address3);
		for(size_t i = 0; i < 50; i++){
			EXPECT_EQ(readb3.get<unsigned char>()[i], 150 + i);
		}
		EXPECT_EQ(Result::Success, storage.commit(readb3));

		EXPECT_EQ(Result::Success, storage.erase(address3));
		EXPECT_THROW(storage.readb(address3), std::logic_error);
	}
	{
  		SimpleFileStorage<FILESIZE> storage{FileRMA<FILESIZE>{filename}};

		StorageBufferRO readb1 = storage.readb(address1);
		for(size_t i = 0; i < 100; i++){
			EXPECT_EQ(readb1.get<unsigned char>()[i], i);
		}
		EXPECT_EQ(Result::Success, storage.commit(readb1));

		StorageBufferRO readb2 = storage.readb(address2);
		for(size_t i = 0; i < address2.size; i++){
			EXPECT_EQ(readb2.get<unsigned char>()[i], 99 + i);
		}
		EXPECT_EQ(Result::Success, storage.commit(readb2));

		EXPECT_THROW(storage.readb(address3), std::logic_error);

		StorageBufferRO readb4 = storage.readb(address4);
		for(size_t i = 0; i < 60; i++){
			EXPECT_EQ(readb4.get<unsigned char>()[i], i);
		}
		EXPECT_EQ(Result::Success, storage.commit(readb4));

		StorageBufferRO readb5 = storage.readb(address5);
		for(size_t i = 0; i < 24; i++){
			EXPECT_EQ(readb5.get<unsigned char>()[i], 60 + i);
		}
		EXPECT_EQ(Result::Success, storage.commit(readb5));
	}
}

//TODO rw addr+offset, read with size_incomplete
//TODO write incorrect
//TODO expand address

}
