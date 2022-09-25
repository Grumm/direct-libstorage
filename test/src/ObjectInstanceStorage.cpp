
#include <memory>

#include <storage/ObjectInstanceStorage.hpp>

#include "gtest/gtest.h"

namespace{

class TestObject{
    int a;
    int b;
public:
    TestObject(int a, int b): a(a), b(b) {}
    int geta() const { return a; }
    int getb() const { return b; }

    bool operator==(const TestObject &other) const {
        return a == other.a && b == other.b;
    }
};

void test_empty_ranges(ObjectInstanceStorage<TestObject> &ois,
                        const std::vector<std::pair<size_t, size_t>> &ranges){
    for(auto r: ranges){
        for(size_t i = r.first; i <= r.second; i++){
            auto [found, ptr] = ois.get(i);
            EXPECT_FALSE(found);
        }
    }
}

void test_nonempty(ObjectInstanceStorage<TestObject> &ois,
                        const std::vector<std::tuple<size_t, size_t, int, int>> &ranges){
    for(auto r: ranges){
        auto [start, end, a, b] = r;
        for(size_t i = start; i <= end; i++){
            auto [found, ptr] = ois.get(i);
            EXPECT_TRUE(found);
            EXPECT_EQ(ptr->geta(), a);
            EXPECT_EQ(ptr->getb(), b);
        }
    }
}

void test_nonempty_ranges(ObjectInstanceStorage<TestObject> &ois,
                        const std::vector<std::tuple<size_t, size_t, int, int>> &ranges){
    for(auto r: ranges){
        auto [start, end, a_start, b_offset] = r;
        for(size_t i = start; i <= end; i++){
            auto [found, ptr] = ois.get(i);
            EXPECT_TRUE(found);
            EXPECT_EQ(ptr->geta(), a_start);
            EXPECT_EQ(ptr->getb(), i - b_offset);
        }
    }
}

TEST(ObjectInstanceStorageTest, AddGetSingle){
    ObjectInstanceStorage<TestObject> ois;
    ois.add(0, TestObject{0,0});
    ois.add(3, TestObject{3,3});
    ois.add(15, TestObject{15,15});

    test_empty_ranges(ois, {{1, 2}, {4, 14}, {16, 50}});
    test_nonempty(ois, {{0, 0, 0, 0}, {3, 3, 3, 3}, {15, 15, 15, 15}});
}

TEST(ObjectInstanceStorageTest, AddGetRange){
    ObjectInstanceStorage<TestObject> ois;
    auto create_obj = [](size_t start, size_t offset){
        return TestObject{(int)start, (int)offset};
    };

    ois.add_range(3, 15, create_obj);
    ois.add_range(20, 40, create_obj);

    test_empty_ranges(ois, {{0, 2}, {16, 19}, {41, 50}});
    test_nonempty_ranges(ois, {{3, 15, 3, 3}, {20, 40, 20, 20}});
}

TEST(ObjectInstanceStorageTest, AddDelSingle){
    ObjectInstanceStorage<TestObject> ois;
    ois.add(0, TestObject{0,0});
    ois.add(3, TestObject{3,3});
    ois.add(15, TestObject{15,15});

    ois.del(3);
    test_empty_ranges(ois, {{1, 14}, {16, 50}});
    test_nonempty(ois, {{0, 0, 0, 0}, {15, 15, 15, 15}});
    ois.del(3);
    ois.del(6);
    test_empty_ranges(ois, {{1, 14}, {16, 50}});
    test_nonempty(ois, {{0, 0, 0, 0}, {15, 15, 15, 15}});

    ois.del(0);
    test_empty_ranges(ois, {{0, 14}, {16, 50}});
    test_nonempty(ois, {{15, 15, 15, 15}});
    ois.del(0);
    test_empty_ranges(ois, {{0, 14}, {16, 50}});
    test_nonempty(ois, {{15, 15, 15, 15}});

    ois.del(15);
    test_empty_ranges(ois, {{0, 50}});
}

TEST(ObjectInstanceStorageTest, AddRangeDel){
    ObjectInstanceStorage<TestObject> ois;
    auto create_obj = [](size_t start, size_t offset){
        return TestObject{(int)start, (int)offset};
    };

    ois.add_range(3, 15, create_obj);
    ois.add_range(20, 40, create_obj);

    ois.del(3);
    test_empty_ranges(ois, {{0, 3}, {16, 19}, {41, 50}});
    test_nonempty_ranges(ois, {{4, 15, 3, 3}, {20, 40, 20, 20}});

    ois.del(6);
    test_empty_ranges(ois, {{0, 3}, {6, 6}, {16, 19}, {41, 50}});
    test_nonempty_ranges(ois, {{4, 5, 3, 3}, {7, 15, 3, 3}, {20, 40, 20, 20}});

    ois.del(15);
    test_empty_ranges(ois, {{0, 3}, {6, 6}, {15, 19}, {41, 50}});
    test_nonempty_ranges(ois, {{4, 5, 3, 3}, {7, 14, 3, 3}, {20, 40, 20, 20}});
}

TEST(ObjectInstanceStorageTest, AddRangeDelRange){
    ObjectInstanceStorage<TestObject> ois;
    auto create_obj = [](size_t start, size_t offset){
        return TestObject{(int)start, (int)offset};
    };

    ois.add_range(4, 5, create_obj);
    ois.add_range(7, 14, create_obj);
    ois.add_range(20, 40, create_obj);

    ois.del_range(7, 14);
    test_empty_ranges(ois, {{0, 3}, {6, 19}, {41, 50}});
    test_nonempty_ranges(ois, {{4, 5, 4, 4}, {20, 40, 20, 20}});

    ois.del_range(20, 25);
    test_empty_ranges(ois, {{0, 3}, {6, 25}, {41, 50}});
    test_nonempty_ranges(ois, {{4, 5, 4, 4}, {26, 40, 20, 20}});

    ois.del_range(35, 40);
    test_empty_ranges(ois, {{0, 3}, {6, 25}, {35, 50}});
    test_nonempty_ranges(ois, {{4, 5, 4, 4}, {26, 34, 20, 20}});

    ois.del_range(1, 4);
    test_empty_ranges(ois, {{0, 4}, {6, 25}, {35, 50}});
    test_nonempty_ranges(ois, {{5, 5, 4, 4}, {26, 34, 20, 20}});

    ois.del_range(30, 40);
    test_empty_ranges(ois, {{0, 4}, {6, 25}, {30, 50}});
    test_nonempty_ranges(ois, {{5, 5, 4, 4}, {26, 29, 20, 20}});

    ois.add_range(7, 14, create_obj);
    test_empty_ranges(ois, {{0, 4}, {6, 6}, {15, 25}, {30, 50}});
    test_nonempty_ranges(ois, {{5, 5, 4, 4}, {7, 14, 7, 7}, {26, 29, 20, 20}});

    ois.del_range(20, 50);
    test_empty_ranges(ois, {{0, 4}, {6, 6}, {15, 50}});
    test_nonempty_ranges(ois, {{5, 5, 4, 4}, {7, 14, 7, 7}});

    ois.del_range(0, 50);
    test_empty_ranges(ois, {{0, 50}});
    test_nonempty_ranges(ois, {});
}

}