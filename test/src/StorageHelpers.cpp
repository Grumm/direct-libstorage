#include "gtest/gtest.h"

#include <storage/StorageHelpers.hpp>

namespace{

using TestObject1 = std::pair<int, int>;

TEST(SetOrderedBySizeTest, AddGetSimple){
    SetOrderedBySize<TestObject1> sobs;
    sobs.add(3, TestObject1{1,3});
    sobs.add(5, TestObject1{5,10});

    EXPECT_TRUE(sobs.has(1));
    EXPECT_TRUE(sobs.has(2));
    EXPECT_TRUE(sobs.has(3));
    EXPECT_TRUE(sobs.has(4));
    EXPECT_TRUE(sobs.has(5));
    EXPECT_FALSE(sobs.has(6));
}

TEST(SetOrderedBySizeTest, AddGetSimpleValues){
    SetOrderedBySize<TestObject1> sobs;
    sobs.add(3, TestObject1{1,3});
    sobs.add(5, TestObject1{5,10});

    {
        SetOrderedBySize<TestObject1> sobs2{sobs};
        EXPECT_EQ(sobs2.get(1), std::make_pair(true, TestObject1{1, 3}));
    }
    {
        SetOrderedBySize<TestObject1> sobs2{sobs};
        EXPECT_EQ(sobs2.get(3), std::make_pair(true, TestObject1{1, 3}));
    }
    {
        SetOrderedBySize<TestObject1> sobs2{sobs};
        EXPECT_EQ(sobs2.get(4), std::make_pair(true, TestObject1{5, 10}));
    }
    {
        SetOrderedBySize<TestObject1> sobs2{sobs};
        EXPECT_EQ(sobs2.get(5), std::make_pair(true, TestObject1{5, 10}));
    }
    {
        SetOrderedBySize<TestObject1> sobs2{sobs};
        EXPECT_FALSE(sobs2.get(6).first);
    }
}

TEST(SetOrderedBySizeTest, AddGetSimpleLimitSize){
    SetOrderedBySize<TestObject1> sobs;
    sobs.add(3, TestObject1{1,3});
    sobs.add(95, TestObject1{5,99});

    EXPECT_TRUE(sobs.has(1, 5));
    EXPECT_TRUE(sobs.has(1, 3));
    EXPECT_FALSE(sobs.has(1, 2));

    EXPECT_TRUE(sobs.has(4, 200));
    EXPECT_TRUE(sobs.has(4, 95));
    EXPECT_FALSE(sobs.has(4, 50));

    {
        SetOrderedBySize<TestObject1> sobs2{sobs};
        EXPECT_FALSE(sobs2.get(4, 50).first);
    }
    {
        SetOrderedBySize<TestObject1> sobs2{sobs};
        EXPECT_EQ(sobs2.get(4, 200), std::make_pair(true, TestObject1{5, 99}));
    }
}


}