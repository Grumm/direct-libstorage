
#include <RandomMemoryAccess.hpp>
#include <cstdio>
#include <memory>

#include "gtest/gtest.h"

namespace{
//GTEST
/*
Scenario is read write:
- multiple ranges
- overlapping ranges
*/

const std::string filename{"/tmp/FileRMA.test"};


enum TestRMAChild{
	File,
	Memory,
};

class RMATestFixture: public ::testing::TestWithParam<TestRMAChild>{
public:
	std::unique_ptr<RMAInterface> rma;

	RMATestFixture(){
		if(GetParam() == TestRMAChild::File){
			::remove(filename.c_str());
			rma = std::unique_ptr<RMAInterface>(new FileRMA<20>(filename));
		} else if (GetParam() == TestRMAChild::Memory){
			rma = std::unique_ptr<RMAInterface>(new MemoryRMA<20>());
		}
	}
	~RMATestFixture(){
		rma.reset();
	}
};

TEST_P(RMATestFixture, ReadWriteRegular){
	std::unique_ptr<unsigned char[]> mem1{new unsigned char[256]};
	std::unique_ptr<unsigned char[]> mem2{new unsigned char[256]};
	StorageBuffer buf1{mem1.get(), 100, 256};
	StorageBuffer buf2{mem2.get(), 0, 256};
	for(size_t i = 0; i < 100; i++){
		mem1[i] = i;
	}
	EXPECT_EQ(Result::Success, rma->write(0, buf1));
	for(size_t i = 0; i < 100; i++){
		mem1[i] = i + 100;
	}
	EXPECT_EQ(Result::Success, rma->write(100, buf1));
	EXPECT_EQ(Result::Success, rma->read(0, 200, buf2));
	for(size_t i = 0; i < 200; i++){
		EXPECT_EQ(mem2[i], i);
	}

	StorageBufferRO readb = rma->readb(0, 200);
	for(size_t i = 0; i < 200; i++){
		EXPECT_EQ(readb.get<unsigned char>()[i], i);
	}
}


TEST_P(RMATestFixture, ReadWriteBuffer){
	StorageBuffer buf1 = rma->writeb(0, 100);
	for(size_t i = 0; i < 100; i++){
		buf1.get<unsigned char>()[i] = i;
	}
	StorageBuffer buf2 = rma->writeb(100, 100);
	for(size_t i = 0; i < 100; i++){
		buf2.get<unsigned char>()[i] = i + 100;
	}
	StorageBufferRO readb = rma->readb(0, 200);
	for(size_t i = 0; i < 200; i++){
		EXPECT_EQ(readb.get<unsigned char>()[i], i);
	}
}

TEST_P(RMATestFixture, ReadWriteNonSequential){
	StorageBuffer buf1 = rma->writeb(0, 100);
	for(size_t i = 0; i < 100; i++){
		buf1.get<unsigned char>()[i] = i;
	}
	StorageBuffer buf2 = rma->writeb(500, 100);
	for(size_t i = 0; i < 100; i++){
		buf2.get<unsigned char>()[i] = 100 + i;
	}
	StorageBuffer buf3 = rma->writeb(9000, 55);
	for(size_t i = 0; i < 55; i++){
		buf3.get<unsigned char>()[i] = 200 + i;
	}

	StorageBufferRO readb1 = rma->readb(0, 100);
	for(size_t i = 0; i < 100; i++){
		EXPECT_EQ(readb1.get<unsigned char>()[i], i);
	}
	StorageBufferRO readb2 = rma->readb(500, 100);
	for(size_t i = 0; i < 100; i++){
		EXPECT_EQ(readb2.get<unsigned char>()[i], 100 + i);
	}
	StorageBufferRO readb3 = rma->readb(9000, 55);
	for(size_t i = 0; i < 55; i++){
		EXPECT_EQ(readb3.get<unsigned char>()[i], 200 + i);
	}
}

INSTANTIATE_TEST_SUITE_P(RMAInterface,
                         RMATestFixture,
                         testing::Values(TestRMAChild::File, TestRMAChild::Memory));

TEST(FileRMAReadWrite, ReadWriteReopen){
	::remove(filename.c_str());
	{
		std::unique_ptr<RMAInterface> rma{new FileRMA(filename)};

		StorageBuffer buf1 = rma->writeb(0, 100);
		for(size_t i = 0; i < 100; i++){
			buf1.get<unsigned char>()[i] = i;
		}
		StorageBuffer buf2 = rma->writeb(500, 100);
		for(size_t i = 0; i < 100; i++){
			buf2.get<unsigned char>()[i] = 100 + i;
		}
		StorageBuffer buf3 = rma->writeb(9000, 55);
		for(size_t i = 0; i < 55; i++){
			buf3.get<unsigned char>()[i] = 200 + i;
		}

		StorageBufferRO readb1 = rma->readb(0, 100);
		for(size_t i = 0; i < 100; i++){
			EXPECT_EQ(readb1.get<unsigned char>()[i], i);
		}
		StorageBufferRO readb2 = rma->readb(500, 100);
		for(size_t i = 0; i < 100; i++){
			EXPECT_EQ(readb2.get<unsigned char>()[i], 100 + i);
		}
		StorageBufferRO readb3 = rma->readb(9000, 55);
		for(size_t i = 0; i < 55; i++){
			EXPECT_EQ(readb3.get<unsigned char>()[i], 200 + i);
		}
	}
	{
		std::unique_ptr<RMAInterface> rma{new FileRMA(filename)};

		StorageBufferRO readb1 = rma->readb(0, 100);
		for(size_t i = 0; i < 100; i++){
			EXPECT_EQ(readb1.get<unsigned char>()[i], i);
		}
		StorageBufferRO readb2 = rma->readb(500, 100);
		for(size_t i = 0; i < 100; i++){
			EXPECT_EQ(readb2.get<unsigned char>()[i], 100 + i);
		}
		StorageBufferRO readb3 = rma->readb(9000, 55);
		for(size_t i = 0; i < 55; i++){
			EXPECT_EQ(readb3.get<unsigned char>()[i], 200 + i);
		}
	}
}

}
