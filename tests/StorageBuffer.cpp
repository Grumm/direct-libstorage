
#include <StorageUtils.hpp>

#include <cstdio>
#include <memory>

#include "gtest/gtest.h"


namespace{

TEST(StorageBufferTest, SimpleConstructCopy){
	std::array<unsigned char, 256> array;
	StorageBuffer sbuf1{array.data(), 32};
	EXPECT_EQ(sbuf1.get(), array.data());
	EXPECT_EQ(sbuf1.size(), 0);
	EXPECT_EQ(sbuf1.allocated(), 32);

	StorageBuffer sbuf2{sbuf1};
	EXPECT_EQ(sbuf2.get(), array.data());
	EXPECT_EQ(sbuf2.size(), 0);
	EXPECT_EQ(sbuf1.allocated(), 32);
}

TEST(StorageBufferTest, ConstructAdvanceReadOffset){
	std::array<unsigned char, 256> array;
	StorageBuffer sbuf1{array.data(), 32};
	auto buf2 = sbuf1.advance(4);
	EXPECT_EQ(sbuf1.get(), array.data());
	EXPECT_EQ(sbuf1.size(), 4);
	EXPECT_EQ(buf2.get(), array.data());
	EXPECT_EQ(buf2.size(), 4);
	EXPECT_EQ(buf2.allocated(), 4);
	buf2.get()[0] = 3;
	buf2.get()[1] = 7;
	buf2.get()[2] = 2;
	buf2.get()[3] = 1;
	EXPECT_EQ(array[0], 3);
	EXPECT_EQ(array[1], 7);
	EXPECT_EQ(array[2], 2);
	EXPECT_EQ(array[3], 1);
	auto buf3 = sbuf1.offset(0, 4);
	EXPECT_EQ(buf3.get(), array.data());
	EXPECT_EQ(buf3.size(), 4);
	EXPECT_EQ(buf3.allocated(), 4);
	EXPECT_EQ(buf3.get()[0], 3);
	EXPECT_EQ(buf3.get()[1], 7);
	EXPECT_EQ(buf3.get()[2], 2);
	EXPECT_EQ(buf3.get()[3], 1);
}

TEST(StorageBufferTest, OffsetAdvance){
	std::array<unsigned char, 256> array;
	StorageBuffer sbuf1{array.data(), 32};
	auto buf2 = sbuf1.advance(6);
	buf2.get()[0] = 3;
	buf2.get()[1] = 7;
	buf2.get()[2] = 2;
	buf2.get()[3] = 1;
	buf2.get()[4] = 10;
	buf2.get()[5] = 222;
	array[6] = 0;
	array[7] = 0;

	size_t offset = 0;
	auto buf3 = buf2.offset_advance(offset, 2);
	EXPECT_EQ(buf3.get()[0], 3);
	EXPECT_EQ(buf3.get()[1], 7);
	buf3 = buf2.offset_advance(offset, 2);
	EXPECT_EQ(buf3.get()[0], 2);
	EXPECT_EQ(buf3.get()[1], 1);
	buf3 = buf2.offset_advance(offset, 2);
	EXPECT_EQ(buf3.get()[0], 10);
	EXPECT_EQ(buf3.get()[1], 222);
	EXPECT_THROW(buf3 = buf2.offset_advance(offset, 2), std::logic_error);
	buf3 = sbuf1.offset_advance(offset, 2);
	EXPECT_EQ(buf3.get()[0], 0);
	EXPECT_EQ(buf3.get()[1], 0);
}

TEST(StorageBufferTest, OffsetAdvanceOverflow){
	std::array<unsigned char, 256> array;
	StorageBuffer sbuf1{array.data(), 32};
	auto buf2 = sbuf1.advance(6);
	buf2.get()[0] = 3;
	buf2.get()[1] = 7;
	buf2.get()[2] = 2;
	buf2.get()[3] = 1;
	buf2.get()[4] = 10;
	buf2.get()[5] = 222;

	size_t offset = 0;
	StorageBuffer<int> buf3;
	EXPECT_THROW(buf3 = buf2.offset_advance(offset, 7), std::logic_error);
	EXPECT_NO_THROW(buf3 = buf2.offset_advance(offset, 6));
	offset = 0;
	EXPECT_THROW(buf3 = sbuf1.offset_advance(offset, 33), std::logic_error);
	EXPECT_NO_THROW(buf3 = sbuf1.offset_advance(offset, 32));
}

TEST(StorageBufferReadOnlyTest, SimpleConstructCopy){
	std::array<unsigned char, 256> array;
	StorageBufferRO sbuf1{array.data(), 32};
	EXPECT_EQ(sbuf1.get(), array.data());
	EXPECT_EQ(sbuf1.size(), 32);

	StorageBufferRO sbuf2{sbuf1};
	EXPECT_EQ(sbuf2.get(), array.data());
	EXPECT_EQ(sbuf2.size(), 32);
}

TEST(StorageBufferReadOnlyTest, ConstructAdvanceReadOffset){
	std::array<unsigned char, 256> array;
	StorageBufferRO sbuf1{array.data(), 32};
	array[0] = 3;
	array[1] = 7;
	array[2] = 2;
	array[3] = 1;
	auto buf2 = sbuf1.offset(0, 4);
	EXPECT_EQ(buf2.get(), array.data());
	EXPECT_EQ(buf2.size(), 4);

	EXPECT_EQ(buf2.get()[0], 3);
	EXPECT_EQ(buf2.get()[1], 7);
	EXPECT_EQ(buf2.get()[2], 2);
	EXPECT_EQ(buf2.get()[3], 1);
}

TEST(StorageBufferReadOnlyTest, OffsetAdvance){
	std::array<unsigned char, 256> array;
	StorageBufferRO sbuf1{array.data(), 32};
	array[0] = 3;
	array[1] = 7;
	array[2] = 2;
	array[3] = 1;
	array[4] = 10;
	array[5] = 222;
	array[6] = 0;
	array[7] = 0;
	auto buf2 = sbuf1.offset(0, 6);

	size_t offset = 0;
	auto buf3 = buf2.offset_advance(offset, 2);
	EXPECT_EQ(buf3.get()[0], 3);
	EXPECT_EQ(buf3.get()[1], 7);
	buf3 = buf2.offset_advance(offset, 2);
	EXPECT_EQ(buf3.get()[0], 2);
	EXPECT_EQ(buf3.get()[1], 1);
	buf3 = buf2.offset_advance(offset, 2);
	EXPECT_EQ(buf3.get()[0], 10);
	EXPECT_EQ(buf3.get()[1], 222);
	EXPECT_THROW(buf3 = buf2.offset_advance(offset, 2), std::logic_error);
	buf3 = sbuf1.offset_advance(offset, 2);
	EXPECT_EQ(buf3.get()[0], 0);
	EXPECT_EQ(buf3.get()[1], 0);
}

TEST(StorageBufferReadOnlyTest, OffsetAdvanceOverflow){
	std::array<unsigned char, 256> array;
	StorageBufferRO sbuf1{array.data(), 32};
	array[0] = 3;
	array[1] = 7;
	array[2] = 2;
	array[3] = 1;
	array[4] = 10;
	array[5] = 222;
	array[6] = 0;
	array[7] = 0;
	auto buf2 = sbuf1.offset(0, 6);

	size_t offset = 0;
	StorageBufferRO<int> buf3{nullptr, 0};
	EXPECT_THROW(buf3 = buf2.offset_advance(offset, 7), std::logic_error);
	EXPECT_NO_THROW(buf3 = buf2.offset_advance(offset, 6));
	offset = 0;
	EXPECT_THROW(buf3 = sbuf1.offset_advance(offset, 33), std::logic_error);
	EXPECT_NO_THROW(buf3 = sbuf1.offset_advance(offset, 32));
}

TEST(StorageBufferReadOnlyTest, ConstructFromWritableBuffer){
	std::array<unsigned char, 256> array;
	StorageBuffer sbuf1{array.data(), 32};
	auto buf2 = sbuf1.advance(6);
	buf2.get()[0] = 3;
	buf2.get()[1] = 7;
	buf2.get()[2] = 2;
	buf2.get()[3] = 1;
	buf2.get()[4] = 10;
	buf2.get()[5] = 222;

	StorageBufferRO<unsigned char> sbuf2{sbuf1};
	EXPECT_EQ(sbuf2.get()[0], 3);
	EXPECT_EQ(sbuf2.get()[1], 7);
	EXPECT_EQ(sbuf2.get()[2], 2);
	EXPECT_EQ(sbuf2.get()[3], 1);
	EXPECT_EQ(sbuf2.get()[4], 10);
	EXPECT_EQ(sbuf2.get()[5], 222);
}

}
