#include <cstdio>
#include <memory>

#include <storage/RandomMemoryAccess.hpp>

#include "gtest/gtest.h"

namespace{
//GTEST
/*
Scenario is read write:
- multiple ranges
- overlapping ranges
*/

const std::string filename{"/tmp/FileRMA.test"};
constexpr size_t FILESIZE = 12;

template<size_t N>
class FileRMATest: public FileRMA<N>{
public:
    FileRMATest(): FileRMA<N>(filename) {}
    virtual ~FileRMATest(){}
};

template<class T>
class RMATest: public ::testing::Test{
public:
    std::unique_ptr<T> rma;

    RMATest(): rma(new T()) {}
    ~RMATest(){
        rma.reset();
    }
};

TYPED_TEST_SUITE_P(RMATest);

TYPED_TEST_P(RMATest, ReadWriteRegular){
    std::unique_ptr<unsigned char[]> mem1{new unsigned char[256]};
    std::unique_ptr<unsigned char[]> mem2{new unsigned char[256]};
    StorageBuffer buf1{mem1.get(), 100, 256};
    StorageBuffer buf2{mem2.get(), 0, 256};
    for(size_t i = 0; i < 100; i++){
        mem1[i] = i;
    }
    EXPECT_EQ(Result::Success, this->rma->write(0, buf1));
    for(size_t i = 0; i < 100; i++){
        mem1[i] = i + 100;
    }
    EXPECT_EQ(Result::Success, this->rma->write(100, buf1));
    EXPECT_EQ(Result::Success, this->rma->read(0, 200, buf2));
    for(size_t i = 0; i < 200; i++){
        EXPECT_EQ(mem2[i], i);
    }

    StorageBufferRO readb = this->rma->readb(0, 200);
    for(size_t i = 0; i < 200; i++){
        EXPECT_EQ(readb.template get<unsigned char>()[i], i);
    }
}


TYPED_TEST_P(RMATest, ReadWriteBuffer){
    StorageBuffer buf1 = this->rma->writeb(0, 100);
    for(size_t i = 0; i < 100; i++){
        buf1.template get<unsigned char>()[i] = i;
    }
    StorageBuffer buf2 = this->rma->writeb(100, 100);
    for(size_t i = 0; i < 100; i++){
        buf2.template get<unsigned char>()[i] = i + 100;
    }
    StorageBufferRO readb = this->rma->readb(0, 200);
    for(size_t i = 0; i < 200; i++){
        EXPECT_EQ(readb.template get<unsigned char>()[i], i);
    }
}

TYPED_TEST_P(RMATest, ReadWriteNonSequential){
    StorageBuffer buf1 = this->rma->writeb(0, 100);
    for(size_t i = 0; i < 100; i++){
        buf1.template get<unsigned char>()[i] = i;
    }
    StorageBuffer buf2 = this->rma->writeb(500, 100);
    for(size_t i = 0; i < 100; i++){
        buf2.template get<unsigned char>()[i] = 100 + i;
    }
    StorageBuffer buf3 = this->rma->writeb(1903, 55);
    for(size_t i = 0; i < 55; i++){
        buf3.template get<unsigned char>()[i] = 200 + i;
    }

    StorageBufferRO readb1 = this->rma->readb(0, 100);
    for(size_t i = 0; i < 100; i++){
        EXPECT_EQ(readb1.template get<unsigned char>()[i], i);
    }
    StorageBufferRO readb2 = this->rma->readb(500, 100);
    for(size_t i = 0; i < 100; i++){
        EXPECT_EQ(readb2.template get<unsigned char>()[i], 100 + i);
    }
    StorageBufferRO readb3 = this->rma->readb(1903, 55);
    for(size_t i = 0; i < 55; i++){
        EXPECT_EQ(readb3.template get<unsigned char>()[i], 200 + i);
    }
}

REGISTER_TYPED_TEST_SUITE_P(RMATest, ReadWriteRegular, ReadWriteBuffer, ReadWriteNonSequential);
INSTANTIATE_TYPED_TEST_SUITE_P(RMATestFile, RMATest, FileRMATest<FILESIZE>);
INSTANTIATE_TYPED_TEST_SUITE_P(RMATestMemory, RMATest, MemoryRMA<FILESIZE>);

TEST(FileRMAReadWrite, ReadWriteReopen){
    std::filesystem::remove(std::filesystem::path{filename});
    {
        auto rma = std::make_unique<FileRMA<FILESIZE>>(filename);

        StorageBuffer buf1 = rma->writeb(0, 100);
        for(size_t i = 0; i < 100; i++){
            buf1.get<unsigned char>()[i] = i;
        }
        StorageBuffer buf2 = rma->writeb(500, 100);
        for(size_t i = 0; i < 100; i++){
            buf2.get<unsigned char>()[i] = 100 + i;
        }
        StorageBuffer buf3 = rma->writeb(1900, 55);
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
        StorageBufferRO readb3 = rma->readb(1900, 55);
        for(size_t i = 0; i < 55; i++){
            EXPECT_EQ(readb3.get<unsigned char>()[i], 200 + i);
        }
    }
    {
        auto rma = std::make_unique<FileRMA<FILESIZE>>(filename);

        StorageBufferRO readb1 = rma->readb(0, 100);
        for(size_t i = 0; i < 100; i++){
            EXPECT_EQ(readb1.get<unsigned char>()[i], i);
        }
        StorageBufferRO readb2 = rma->readb(500, 100);
        for(size_t i = 0; i < 100; i++){
            EXPECT_EQ(readb2.get<unsigned char>()[i], 100 + i);
        }
        StorageBufferRO readb3 = rma->readb(1900, 55);
        for(size_t i = 0; i < 55; i++){
            EXPECT_EQ(readb3.get<unsigned char>()[i], 200 + i);
        }
    }
}

}
