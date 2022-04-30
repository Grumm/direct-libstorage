
#include <SimpleStorage.hpp>
#include <cstdio>
#include <memory>

#include "gtest/gtest.h"

namespace{

TEST(VirtAddressMapping, PutGetSimple){
	VirtAddressMapping vam;

	uint64_t addr1 = 0x10;
	size_t size1 = 5;
	size_t offset1 = vam.alloc(addr1, size1);

	{
		auto [offset2, size2] = vam.lookup(addr1, size1);
		EXPECT_EQ(offset1, offset2);
		EXPECT_EQ(size1, size2);
	}
	{
		auto [offset2, size2] = vam.lookup(addr1, size1);
		EXPECT_EQ(offset1, offset2);
		EXPECT_EQ(size1, size2);
	}
}

TEST(VirtAddressMapping, PutGetMultiple){
	VirtAddressMapping vam;

	uint64_t addr1 = 0x10;
	size_t size1 = 5;
	size_t offset1 = vam.alloc(addr1, size1);

	uint64_t addr2 = 0x30;
	size_t size2 = 1;
	size_t offset2 = vam.alloc(addr2, size2);

	{
		auto [offset, size] = vam.lookup(addr1, size1);
		EXPECT_EQ(offset1, offset);
		EXPECT_EQ(size1, size);
	}
	{
		auto [offset, size] = vam.lookup(addr2, size2);
		EXPECT_EQ(offset2, offset);
		EXPECT_EQ(size2, size);
	}
	{
		auto [offset, size] = vam.lookup(addr1, size1);
		EXPECT_EQ(offset1, offset);
		EXPECT_EQ(size1, size);
	}
	{
		auto [offset, size] = vam.lookup(addr2, size2);
		EXPECT_EQ(offset2, offset);
		EXPECT_EQ(size2, size);
	}
}

TEST(VirtAddressMapping, PutGetMultipleIncompleteRange){
	VirtAddressMapping vam;

	uint64_t addr1 = 0x10;
	size_t size1 = 5;
	size_t size1_incomplete = 3;
	size_t offset1 = vam.alloc(addr1, size1);

	uint64_t addr2 = 0x30;
	size_t size2 = 2;
	size_t size2_incomplete = 1;
	size_t offset2 = vam.alloc(addr2, size2);

	{
		auto [offset, size] = vam.lookup(addr1, size1_incomplete);
		EXPECT_EQ(offset1, offset);
		EXPECT_EQ(size1, size);
	}
	{
		auto [offset, size] = vam.lookup(addr2, size2_incomplete);
		EXPECT_EQ(offset2, offset);
		EXPECT_EQ(size2, size);
	}
	{
		auto [offset, size] = vam.lookup(addr1, size1);
		EXPECT_EQ(offset1, offset);
		EXPECT_EQ(size1, size);
	}
	{
		auto [offset, size] = vam.lookup(addr2, size2);
		EXPECT_EQ(offset2, offset);
		EXPECT_EQ(size2, size);
	}
}

TEST(VirtAddressMapping, GetOnly){
	VirtAddressMapping vam;

	uint64_t addr1 = 0x10;
	size_t size1 = 5;
	size_t size1_incomplete = 3;
	EXPECT_EQ(Result::Success, vam.alloc_unmapped(addr1, size1));
	{
		auto [offset, size] = vam.lookup(addr1, size1);
		EXPECT_EQ(0, offset);
		EXPECT_EQ(size1, size);
	}
}

TEST(VirtAddressMapping, PutGetMultipleAdjacentRanges){
	VirtAddressMapping vam;

	uint64_t addr1 = 0x10;
	size_t size1 = 5;
	size_t size1_incomplete = 3;
	size_t offset1 = vam.alloc(addr1, size1);

	uint64_t addr2 = 0x10 + 5;
	size_t size2 = 2;
	size_t size2_incomplete = 1;
	size_t offset2 = vam.alloc(addr2, size2);

	uint64_t addr3 = 0x10 + 5 + 2;
	size_t size3 = 6;
	size_t size3_incomplete = 3;
	size_t offset3 = vam.alloc(addr3, size3);

	{
		auto [offset, size] = vam.lookup(addr1, size1);
		EXPECT_EQ(offset1, offset);
		EXPECT_EQ(size1, size);
	}
	{
		auto [offset, size] = vam.lookup(addr2, size2);
		EXPECT_EQ(offset2, offset);
		EXPECT_EQ(size2, size);
	}
	{
		auto [offset, size] = vam.lookup(addr3, size3);
		EXPECT_EQ(offset3, offset);
		EXPECT_EQ(size3, size);
	}
	{
		auto [offset, size] = vam.lookup(addr1, size1);
		EXPECT_EQ(offset1, offset);
		EXPECT_EQ(size1, size);
	}
	{
		auto [offset, size] = vam.lookup(addr2, size2);
		EXPECT_EQ(offset2, offset);
		EXPECT_EQ(size2, size);
	}
	{
		auto [offset, size] = vam.lookup(addr3, size3);
		EXPECT_EQ(offset3, offset);
		EXPECT_EQ(size3, size);
	}
}

TEST(VirtAddressMapping, PutGetMultipleAdjacentRangesIncomplete){
	VirtAddressMapping vam;

	uint64_t addr1 = 0x10;
	size_t size1 = 5;
	size_t size1_incomplete = 3;
	size_t offset1 = vam.alloc(addr1, size1);

	uint64_t addr2 = addr1 + size1;
	size_t size2 = 2;
	size_t size2_incomplete = 1;
	size_t offset2 = vam.alloc(addr2, size2);

	uint64_t addr3 = addr2 + size2;
	size_t size3 = 6;
	size_t size3_incomplete = 3;
	size_t offset3 = vam.alloc(addr3, size3);

	{
		auto [offset, size] = vam.lookup(addr1, size1_incomplete);
		EXPECT_EQ(offset1, offset);
		EXPECT_EQ(size1, size);
	}
	{
		auto [offset, size] = vam.lookup(addr2, size2_incomplete);
		EXPECT_EQ(offset2, offset);
		EXPECT_EQ(size2, size);
	}
	{
		auto [offset, size] = vam.lookup(addr3, size3_incomplete);
		EXPECT_EQ(offset3, offset);
		EXPECT_EQ(size3, size);
	}
	{
		auto [offset, size] = vam.lookup(addr1, size1);
		EXPECT_EQ(offset1, offset);
		EXPECT_EQ(size1, size);
	}
	{
		auto [offset, size] = vam.lookup(addr2, size2);
		EXPECT_EQ(offset2, offset);
		EXPECT_EQ(size2, size);
	}
	{
		auto [offset, size] = vam.lookup(addr3, size3);
		EXPECT_EQ(offset3, offset);
		EXPECT_EQ(size3, size);
	}
}

TEST(VirtAddressMapping, PutGetMultipleAdjacentRangesAddressOffset){
	VirtAddressMapping vam;

	uint64_t addr1 = 0x10;
	size_t size1 = 5;
	size_t size1_incomplete = 3;
	size_t addr1_offset = 2;
	size_t offset1 = vam.alloc(addr1, size1);

	uint64_t addr2 = 0x10 + 5;
	size_t addr2_offset = 1;
	size_t size2 = 2;
	size_t size2_incomplete = 1;
	size_t offset2 = vam.alloc(addr2, size2);

	uint64_t addr3 = 0x10 + 5 + 2;
	size_t addr3_offset = 2;
	size_t size3 = 6;
	size_t size3_incomplete = 3;
	size_t offset3 = vam.alloc(addr3, size3);

	{
		auto [offset, size] = vam.lookup(addr1 + addr1_offset, size1 - addr1_offset);
		EXPECT_EQ(offset1 + addr1_offset, offset);
		EXPECT_EQ(size1 - addr1_offset, size);
	}
	{
		auto [offset, size] = vam.lookup(addr2 + addr2_offset, size2 - addr2_offset);
		EXPECT_EQ(offset2 + addr2_offset, offset);
		EXPECT_EQ(size2 - addr2_offset, size);
	}
	{
		auto [offset, size] = vam.lookup(addr3 + addr3_offset, size3 - addr3_offset);
		EXPECT_EQ(offset3 + addr3_offset, offset);
		EXPECT_EQ(size3 - addr3_offset, size);
	}
	{
		auto [offset, size] = vam.lookup(addr1 + addr1_offset, size1 - addr1_offset);
		EXPECT_EQ(offset1 + addr1_offset, offset);
		EXPECT_EQ(size1 - addr1_offset, size);
	}
	{
		auto [offset, size] = vam.lookup(addr2 + addr2_offset, size2 - addr2_offset);
		EXPECT_EQ(offset2 + addr2_offset, offset);
		EXPECT_EQ(size2 - addr2_offset, size);
	}
	{
		auto [offset, size] = vam.lookup(addr3 + addr3_offset, size3 - addr3_offset);
		EXPECT_EQ(offset3 + addr3_offset, offset);
		EXPECT_EQ(size3 - addr3_offset, size);
	}
}

TEST(VirtAddressMapping, PutGetMultipleAdjacentRangesIncompleteAddressOffset){
	VirtAddressMapping vam;

	uint64_t addr1 = 0x10;
	size_t size1 = 5;
	size_t size1_incomplete = 3;
	size_t addr1_offset = 2;
	size_t offset1 = vam.alloc(addr1, size1);

	uint64_t addr2 = 0x10 + 5;
	size_t addr2_offset = 1;
	size_t size2 = 2;
	size_t size2_incomplete = 1;
	size_t offset2 = vam.alloc(addr2, size2);

	uint64_t addr3 = 0x10 + 5 + 2;
	size_t addr3_offset = 2;
	size_t size3 = 6;
	size_t size3_incomplete = 3;
	size_t offset3 = vam.alloc(addr3, size3);

	{
		auto [offset, size] = vam.lookup(addr1 + addr1_offset, size1_incomplete);
		EXPECT_EQ(offset1 + addr1_offset, offset);
		EXPECT_EQ(size1 - addr1_offset, size);
	}
	{
		auto [offset, size] = vam.lookup(addr2 + addr2_offset, size2_incomplete);
		EXPECT_EQ(offset2 + addr2_offset, offset);
		EXPECT_EQ(size2 - addr2_offset, size);
	}
	{
		auto [offset, size] = vam.lookup(addr3 + addr3_offset, size3_incomplete);
		EXPECT_EQ(offset3 + addr3_offset, offset);
		EXPECT_EQ(size3 - addr3_offset, size);
	}
	{
		auto [offset, size] = vam.lookup(addr1, size1);
		EXPECT_EQ(offset1, offset);
		EXPECT_EQ(size1, size);
	}
	{
		auto [offset, size] = vam.lookup(addr2, size2);
		EXPECT_EQ(offset2, offset);
		EXPECT_EQ(size2, size);
	}
	{
		auto [offset, size] = vam.lookup(addr3, size3);
		EXPECT_EQ(offset3, offset);
		EXPECT_EQ(size3, size);
	}
}

TEST(VirtAddressMapping, PutGetMultipleIncorrectSize){
	VirtAddressMapping vam;

	uint64_t addr1 = 0x10;
	size_t size1 = 5;
	size_t size1_incomplete = 3;
	size_t addr1_offset = 2;
	size_t offset1 = vam.alloc(addr1, size1);

	uint64_t addr2 = 0x10 + 5;
	size_t addr2_offset = 1;
	size_t size2 = 2;
	size_t size2_incomplete = 1;
	size_t offset2 = vam.alloc(addr2, size2);

	uint64_t addr3 = 0x10 + 5 + 2;
	size_t addr3_offset = 2;
	size_t size3 = 6;
	size_t size3_incomplete = 3;
	size_t offset3 = vam.alloc(addr3, size3);

	EXPECT_THROW({
		vam.lookup(addr1, size1 + 1);
	}, std::logic_error);
	EXPECT_THROW({
		vam.lookup(addr2, size2 + 1);
	}, std::logic_error);
	EXPECT_THROW({
		vam.lookup(addr3, size3 + 1);
	}, std::logic_error);
	EXPECT_THROW({
		vam.lookup(addr1 + addr1_offset, size1);
	}, std::logic_error);
	EXPECT_THROW({
		vam.lookup(addr2 + addr2_offset, size2);
	}, std::logic_error);
	EXPECT_THROW({
		vam.lookup(addr3 + addr3_offset, size3);
	}, std::logic_error);
}

TEST(VirtAddressMapping, PutGetIncorrect){
	VirtAddressMapping vam;

	uint64_t addr1 = 0x10;
	size_t size1 = 5;
	size_t size1_incomplete = 3;
	size_t offset1 = vam.alloc(addr1, size1);

	uint64_t addr2 = addr1 + size1;
	size_t size2 = 2;
	size_t size2_incomplete = 1;
	size_t offset2 = vam.alloc(addr2, size2);

	uint64_t addr3 = addr2 + size2;
	size_t size3 = 6;
	size_t size3_incomplete = 3;
	size_t offset3 = vam.alloc(addr3, size3);

	{
		auto [offset, size] = vam.lookup(addr1, size1_incomplete);
		EXPECT_EQ(offset1, offset);
		EXPECT_EQ(size1, size);
	}
	{
		auto [offset, size] = vam.lookup(addr2, size2_incomplete);
		EXPECT_EQ(offset2, offset);
		EXPECT_EQ(size2, size);
	}
	{
		auto [offset, size] = vam.lookup(addr3, size3_incomplete);
		EXPECT_EQ(offset3, offset);
		EXPECT_EQ(size3, size);
	}
	{
		auto [offset, size] = vam.lookup(addr1, size1);
		EXPECT_EQ(offset1, offset);
		EXPECT_EQ(size1, size);
	}
	{
		auto [offset, size] = vam.lookup(addr2, size2);
		EXPECT_EQ(offset2, offset);
		EXPECT_EQ(size2, size);
	}
	{
		auto [offset, size] = vam.lookup(addr3, size3);
		EXPECT_EQ(offset3, offset);
		EXPECT_EQ(size3, size);
	}
}

//TODO expand
//TODO del
//TODO deserialize

}
