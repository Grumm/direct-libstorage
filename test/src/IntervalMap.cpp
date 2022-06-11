
#include <memory>

#include <storage/IntervalMap.hpp>

#include "gtest/gtest.h"

namespace{

size_t range2val(size_t start, size_t end){
    return 10000000 + start * 1000 + end;
}

class IntervalMapTest: public ::testing::Test{
    IntervalMap<size_t, size_t> im;
public:
    enum class IntervalOp{
        Delete,
        Set,
        Find,
        Insert,
        Has,
        Has2,
    };

    void validate_range(size_t interval_start, size_t interval_end){

    }

    void validate_intervals(const std::vector<std::tuple<IntervalOp, size_t, size_t, ssize_t, bool>> &op_intervals){
        for(auto &interval: op_intervals){
            auto [op, interval_start, interval_end, val, result] = interval;
            if(val == -1){
                val = range2val(interval_start, interval_end);
            }
            switch(op){
                case IntervalOp::Set:
                {
                    EXPECT_EQ(im.set(interval_start, interval_end, val), Result::Success);
                    EXPECT_TRUE(im.has(interval_start, interval_end));
                    break;
                }
                case IntervalOp::Insert:
                {
                    EXPECT_EQ(im.insert(interval_start, interval_end, val), Result::Success);
                    EXPECT_TRUE(im.has(interval_start, interval_end));
                    break;
                }
                case IntervalOp::Has:
                {
                    EXPECT_EQ(im.has(interval_start), result);
                    EXPECT_EQ(im.has(interval_start, interval_start), result);
                    break;
                }
                case IntervalOp::Has2:
                {
                    EXPECT_EQ(im.has(interval_start, interval_end), result);
                    break;
                }
                case IntervalOp::Delete:
                {
                    EXPECT_EQ(im.del(interval_start, interval_end), Result::Success);
                    EXPECT_FALSE(im.has(interval_start, interval_end));
                    for(size_t i = interval_start; i <= interval_end; i++){
                        auto iv_result = im.find(i);
                        EXPECT_FALSE(iv_result.found());
                    }
                    break;
                }
                case IntervalOp::Find:
                {
                    //[interval_start, interval_end] -> val
                    EXPECT_EQ(im.has(interval_start, interval_end), result);
                    for(size_t i = interval_start; i <= interval_end; i++){
                        auto iv_result = im.find(i);
                        if(result){
                            auto [iv_start, iv_end] = iv_result.range();
                            auto vref = iv_result.value();

                            EXPECT_TRUE(iv_result.found());
                            EXPECT_EQ(iv_start, interval_start);
                            EXPECT_EQ(iv_end, interval_end);
                            EXPECT_EQ(vref, val);
                        } else {
                            EXPECT_FALSE(iv_result.found());
                        }
                    }
                    break;
                }
            }
        }
    }
};

TEST_F(IntervalMapTest, SimpleRangeInsertFind){
    validate_intervals({
        {IntervalOp::Find, 0, 100, -1, false},
        {IntervalOp::Insert, 1, 1, -1, false},
        {IntervalOp::Find, 0, 0, -1, false},
        {IntervalOp::Find, 1, 1, -1, true},
        {IntervalOp::Find, 2, 100, -1, false},
    });
    validate_intervals({
        {IntervalOp::Insert, 3, 7, -1, false},
        {IntervalOp::Find, 0, 0, -1, false},
        {IntervalOp::Find, 1, 1, -1, true},
        {IntervalOp::Find, 2, 2, -1, false},
        {IntervalOp::Find, 3, 7, -1, true},
        {IntervalOp::Find, 8, 100, -1, false},
    });
}

TEST_F(IntervalMapTest, SimpleRangeInsertFindDel){
    validate_intervals({
        {IntervalOp::Insert, 3, 7, -1, false},
        {IntervalOp::Delete, 3, 7, -1, false},
        {IntervalOp::Find, 0, 100, -1, false},
        {IntervalOp::Insert, 4, 7, -1, false},
        {IntervalOp::Find, 0, 3, -1, false},
        {IntervalOp::Find, 4, 7, -1, true},
        {IntervalOp::Find, 8, 100, -1, false},
    });
    validate_intervals({
        {IntervalOp::Insert, 9, 10, -1, false},
        {IntervalOp::Delete, 3, 5, -1, false},
        {IntervalOp::Find, 0, 5, -1, false},
        {IntervalOp::Find, 6, 7, range2val(4, 7), true},
        {IntervalOp::Find, 8, 8, -1, false},
        {IntervalOp::Find, 9, 10, -1, true},
        {IntervalOp::Find, 11, 100, -1, false},
    });
    validate_intervals({
        {IntervalOp::Delete, 6, 7, -1, false},
        {IntervalOp::Delete, 9, 10, -1, false},
        {IntervalOp::Find, 0, 100, -1, false},
    });
}

TEST_F(IntervalMapTest, DeleteStartingBeforeLeftBorder){
    validate_intervals({
        {IntervalOp::Delete, 0, 100, -1, false},
        {IntervalOp::Insert, 0, 5, -1, false},
        {IntervalOp::Insert, 10, 30, -1, false},
        {IntervalOp::Insert, 50, 70, -1, false},
        {IntervalOp::Insert, 80, 90, -1, false},
    });
    validate_intervals({
        {IntervalOp::Delete, 6, 8, -1, false},
        {IntervalOp::Find, 0, 5, -1, true},
        {IntervalOp::Find, 6, 9, -1, false},
        {IntervalOp::Find, 10, 30, -1, true},
    });
    validate_intervals({
        {IntervalOp::Delete, 7, 10, -1, false},
        {IntervalOp::Find, 0, 5, -1, true},
        {IntervalOp::Find, 6, 10, -1, false},
        {IntervalOp::Find, 11, 30, range2val(10, 30), true},
        {IntervalOp::Find, 31, 49, -1, false},
    });
    validate_intervals({
        {IntervalOp::Delete, 7, 15, -1, false},
        {IntervalOp::Find, 0, 5, -1, true},
        {IntervalOp::Find, 6, 15, -1, false},
        {IntervalOp::Find, 16, 30, range2val(10, 30), true},
        {IntervalOp::Find, 31, 49, -1, false},
    });
}

TEST_F(IntervalMapTest, DeleteStartingWithLeftBorder){
    validate_intervals({
        {IntervalOp::Delete, 0, 100, -1, false},
        {IntervalOp::Insert, 0, 5, -1, false},
        {IntervalOp::Insert, 10, 30, -1, false},
        {IntervalOp::Insert, 50, 70, -1, false},
        {IntervalOp::Insert, 80, 90, -1, false},
        {IntervalOp::Insert, 95, 100, -1, false},
        {IntervalOp::Insert, 113, 142, -1, false},
    });
    validate_intervals({
        {IntervalOp::Delete, 10, 20, -1, false},
        {IntervalOp::Find, 0, 5, -1, true},
        {IntervalOp::Find, 6, 20, -1, false},
        {IntervalOp::Find, 21, 30, range2val(10, 30), true},
        {IntervalOp::Find, 31, 49, -1, false},
    });
    validate_intervals({
        {IntervalOp::Delete, 21, 30, -1, false},
        {IntervalOp::Find, 0, 5, -1, true},
        {IntervalOp::Find, 6, 49, -1, false},
    });
    validate_intervals({
        {IntervalOp::Delete, 50, 75, -1, false},
        {IntervalOp::Find, 0, 5, -1, true},
        {IntervalOp::Find, 6, 79, -1, false},
        {IntervalOp::Find, 80, 90, -1, true},
        {IntervalOp::Find, 91, 94, -1, false},
    });
    validate_intervals({
        {IntervalOp::Delete, 80, 97, -1, false},
        {IntervalOp::Find, 0, 5, -1, true},
        {IntervalOp::Find, 6, 97, -1, false},
        {IntervalOp::Find, 98, 100, range2val(95, 100), true},
        {IntervalOp::Find, 101, 112, -1, false},
    });
}

TEST_F(IntervalMapTest, DeleteStartingInCenter){
    validate_intervals({
        {IntervalOp::Delete, 0, 100, -1, false},
        {IntervalOp::Insert, 0, 5, -1, false},
        {IntervalOp::Insert, 10, 30, -1, false},
        {IntervalOp::Insert, 50, 70, -1, false},
        {IntervalOp::Insert, 80, 90, -1, false},
    });
    validate_intervals({
        {IntervalOp::Delete, 17, 21, -1, false},
        {IntervalOp::Find, 0, 5, -1, true},
        {IntervalOp::Find, 6, 9, -1, false},
        {IntervalOp::Find, 10, 16, range2val(10, 30), true},
        {IntervalOp::Find, 22, 30, range2val(10, 30), true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 70, -1, true},
    });
    validate_intervals({
        {IntervalOp::Delete, 26, 30, -1, false},
        {IntervalOp::Find, 0, 5, -1, true},
        {IntervalOp::Find, 6, 9, -1, false},
        {IntervalOp::Find, 10, 16, range2val(10, 30), true},
        {IntervalOp::Find, 22, 25, range2val(10, 30), true},
        {IntervalOp::Find, 26, 49, -1, false},
        {IntervalOp::Find, 50, 70, -1, true},
    });
    validate_intervals({
        {IntervalOp::Delete, 13, 18, -1, false},
        {IntervalOp::Find, 0, 5, -1, true},
        {IntervalOp::Find, 6, 9, -1, false},
        {IntervalOp::Find, 10, 12, range2val(10, 30), true},
        {IntervalOp::Find, 22, 25, range2val(10, 30), true},
        {IntervalOp::Find, 26, 49, -1, false},
        {IntervalOp::Find, 50, 70, -1, true},
    });
    validate_intervals({
        {IntervalOp::Delete, 23, 50, -1, false},
        {IntervalOp::Find, 0, 5, -1, true},
        {IntervalOp::Find, 6, 9, -1, false},
        {IntervalOp::Find, 10, 12, range2val(10, 30), true},
        {IntervalOp::Find, 13, 21, -1, false},
        {IntervalOp::Find, 22, 22, range2val(10, 30), true},
        {IntervalOp::Find, 23, 50, -1, false},
        {IntervalOp::Find, 51, 70, range2val(50, 70), true},
    });
    validate_intervals({
        {IntervalOp::Delete, 22, 22, -1, false},
        {IntervalOp::Find, 0, 5, -1, true},
        {IntervalOp::Find, 6, 9, -1, false},
        {IntervalOp::Find, 10, 12, range2val(10, 30), true},
        {IntervalOp::Find, 13, 50, -1, false},
        {IntervalOp::Find, 51, 70, range2val(50, 70), true},
    });
}

TEST_F(IntervalMapTest, DeleteStartingWithRightBorder){
    validate_intervals({
        {IntervalOp::Delete, 0, 200, -1, false},
        {IntervalOp::Insert, 10, 30, -1, false},
        {IntervalOp::Insert, 50, 70, -1, false},
        {IntervalOp::Insert, 80, 90, -1, false},
    });
    validate_intervals({
        {IntervalOp::Delete, 30, 40, -1, false},
        {IntervalOp::Find, 10, 29, range2val(10, 30), true},
        {IntervalOp::Find, 30, 49, -1, false},
    });
    validate_intervals({
        {IntervalOp::Delete, 29, 50, -1, false},
        {IntervalOp::Find, 10, 28, range2val(10, 30), true},
        {IntervalOp::Find, 29, 50, -1, false},
        {IntervalOp::Find, 51, 70, range2val(50, 70), true},
    });
    validate_intervals({
        {IntervalOp::Delete, 28, 60, -1, false},
        {IntervalOp::Find, 10, 27, range2val(10, 30), true},
        {IntervalOp::Find, 28, 60, -1, false},
        {IntervalOp::Find, 61, 70, range2val(50, 70), true},
    });
}

TEST_F(IntervalMapTest, DeleteMultipleRangesStartingNotInRange){
    validate_intervals({
        {IntervalOp::Delete, 0, 200, -1, false},
        {IntervalOp::Insert, 10, 15, -1, false},
        {IntervalOp::Insert, 20, 30, -1, false},
        {IntervalOp::Insert, 50, 60, -1, false},
        {IntervalOp::Insert, 65, 70, -1, false},
        {IntervalOp::Insert, 80, 90, -1, false},
    });
    validate_intervals({
        {IntervalOp::Delete, 2, 17, -1, false},
        {IntervalOp::Find, 0, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
    });
    validate_intervals({
        {IntervalOp::Delete, 40, 62, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 64, -1, false},
        {IntervalOp::Find, 65, 70, -1, true},
        {IntervalOp::Insert, 50, 60, -1, false},
    });
    validate_intervals({
        {IntervalOp::Delete, 40, 77, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 79, -1, false},
        {IntervalOp::Find, 80, 90, -1, true},
    });
}

TEST_F(IntervalMapTest, InsertOnBorderFromLeftDifferentValue){
    validate_intervals({
        {IntervalOp::Insert, 10, 15, -1, false},
        {IntervalOp::Insert, 20, 30, -1, false},
        {IntervalOp::Insert, 50, 60, -1, false},
    });
    validate_intervals({
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 15, -1, true},
        {IntervalOp::Find, 16, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 100, -1, false},
    });
    validate_intervals({ //start
        {IntervalOp::Insert, 5, 9, -1, false},
        {IntervalOp::Find, 0, 4, -1, false},
        {IntervalOp::Find, 5, 9, -1, true},
        {IntervalOp::Find, 10, 15, -1, true},
        {IntervalOp::Find, 16, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 100, -1, false},
    });
    validate_intervals({ //middle
        {IntervalOp::Insert, 17, 19, -1, false},
        {IntervalOp::Find, 0, 4, -1, false},
        {IntervalOp::Find, 5, 9, -1, true},
        {IntervalOp::Find, 10, 15, -1, true},
        {IntervalOp::Find, 16, 16, -1, false},
        {IntervalOp::Find, 17, 19, -1, true},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 100, -1, false},
    });
    validate_intervals({ //end
        {IntervalOp::Insert, 45, 49, -1, false},
        {IntervalOp::Find, 0, 4, -1, false},
        {IntervalOp::Find, 5, 9, -1, true},
        {IntervalOp::Find, 10, 15, -1, true},
        {IntervalOp::Find, 16, 16, -1, false},
        {IntervalOp::Find, 17, 19, -1, true},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 44, -1, false},
        {IntervalOp::Find, 45, 49, -1, true},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 100, -1, false},
    });
}

TEST_F(IntervalMapTest, InsertOnBorderFromLeftSameValue){
    validate_intervals({
        {IntervalOp::Insert, 10, 15, -1, false},
        {IntervalOp::Insert, 20, 30, -1, false},
        {IntervalOp::Insert, 50, 60, -1, false},
    });
    validate_intervals({
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 15, -1, true},
        {IntervalOp::Find, 16, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 100, -1, false},
    });
    validate_intervals({ //start
        {IntervalOp::Insert, 5, 9, range2val(10, 15), false},
        {IntervalOp::Find, 0, 4, -1, false},
        {IntervalOp::Find, 5, 15, range2val(10, 15), true},
        {IntervalOp::Find, 16, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 100, -1, false},
    });
    validate_intervals({ //middle
        {IntervalOp::Insert, 17, 19, range2val(20, 30), false},
        {IntervalOp::Find, 0, 4, -1, false},
        {IntervalOp::Find, 5, 15, range2val(10, 15), true},
        {IntervalOp::Find, 16, 16, -1, false},
        {IntervalOp::Find, 17, 30, range2val(20, 30), true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 100, -1, false},
    });
    validate_intervals({ //end
        {IntervalOp::Insert, 45, 49, range2val(50, 60), false},
        {IntervalOp::Find, 0, 4, -1, false},
        {IntervalOp::Find, 5, 15, range2val(10, 15), true},
        {IntervalOp::Find, 16, 16, -1, false},
        {IntervalOp::Find, 17, 30, range2val(20, 30), true},
        {IntervalOp::Find, 31, 44, -1, false},
        {IntervalOp::Find, 45, 60, range2val(50, 60), true},
        {IntervalOp::Find, 61, 100, -1, false},
    });
}

TEST_F(IntervalMapTest, InsertOnBorderFromRightDifferentValue){
    validate_intervals({
        {IntervalOp::Insert, 10, 15, -1, false},
        {IntervalOp::Insert, 20, 30, -1, false},
        {IntervalOp::Insert, 50, 60, -1, false},
    });
    validate_intervals({
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 15, -1, true},
        {IntervalOp::Find, 16, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 100, -1, false},
    });
    validate_intervals({ //start
        {IntervalOp::Insert, 16, 18, -1, false},
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 15, -1, true},
        {IntervalOp::Find, 16, 18, -1, true},
        {IntervalOp::Find, 19, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 100, -1, false},
    });
    validate_intervals({ //middle
        {IntervalOp::Insert, 31, 35, -1, false},
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 15, -1, true},
        {IntervalOp::Find, 16, 18, -1, true},
        {IntervalOp::Find, 19, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 35, -1, true},
        {IntervalOp::Find, 36, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 100, -1, false},
    });
    validate_intervals({ //end
        {IntervalOp::Insert, 61, 70, -1, false},
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 15, -1, true},
        {IntervalOp::Find, 16, 18, -1, true},
        {IntervalOp::Find, 19, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 35, -1, true},
        {IntervalOp::Find, 36, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 70, -1, true},
        {IntervalOp::Find, 71, 100, -1, false},
    });
}

TEST_F(IntervalMapTest, InsertOnBorderFromRightSameValue){
    validate_intervals({
        {IntervalOp::Insert, 10, 15, -1, false},
        {IntervalOp::Insert, 20, 30, -1, false},
        {IntervalOp::Insert, 50, 60, -1, false},
    });
    validate_intervals({
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 15, -1, true},
        {IntervalOp::Find, 16, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 100, -1, false},
    });
    validate_intervals({ //start
        {IntervalOp::Insert, 16, 18, range2val(10, 15), false},
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 18, range2val(10, 15), true},
        {IntervalOp::Find, 19, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 100, -1, false},
    });
    validate_intervals({ //middle
        {IntervalOp::Insert, 31, 35, range2val(20, 30), false},
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 18, range2val(10, 15), true},
        {IntervalOp::Find, 19, 19, -1, false},
        {IntervalOp::Find, 20, 35, range2val(20, 30), true},
        {IntervalOp::Find, 36, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 100, -1, false},
    });
    validate_intervals({ //end
        {IntervalOp::Insert, 61, 70, range2val(50, 60), false},
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 18, range2val(10, 15), true},
        {IntervalOp::Find, 19, 19, -1, false},
        {IntervalOp::Find, 20, 35, range2val(20, 30), true},
        {IntervalOp::Find, 36, 49, -1, false},
        {IntervalOp::Find, 50, 70, range2val(50, 60), true},
        {IntervalOp::Find, 71, 100, -1, false},
    });
}

TEST_F(IntervalMapTest, InsertOnBorderInBetweenDifferentValue){
    validate_intervals({
        {IntervalOp::Insert, 10, 15, -1, false},
        {IntervalOp::Insert, 20, 30, -1, false},
        {IntervalOp::Insert, 50, 60, -1, false},
        {IntervalOp::Insert, 80, 90, -1, false},
    });
    validate_intervals({
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 15, -1, true},
        {IntervalOp::Find, 16, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 79, -1, false},
        {IntervalOp::Find, 80, 90, -1, true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
    validate_intervals({ //start
        {IntervalOp::Insert, 16, 19, -1, false},
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 15, -1, true},
        {IntervalOp::Find, 16, 19, -1, true},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 79, -1, false},
        {IntervalOp::Find, 80, 90, -1, true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
    validate_intervals({ //end
        {IntervalOp::Insert, 61, 79, -1, false},
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 15, -1, true},
        {IntervalOp::Find, 16, 19, -1, true},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 79, -1, true},
        {IntervalOp::Find, 80, 90, -1, true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
    validate_intervals({
        {IntervalOp::Delete, 0, 100, -1, false},
        {IntervalOp::Insert, 10, 15, -1, false},
        {IntervalOp::Insert, 20, 30, -1, false},
        {IntervalOp::Insert, 50, 60, -1, false},
        {IntervalOp::Insert, 80, 90, -1, false},
    });
    validate_intervals({ //middle
        {IntervalOp::Insert, 31, 49, -1, false},
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 15, -1, true},
        {IntervalOp::Find, 16, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, true},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 79, -1, false},
        {IntervalOp::Find, 80, 90, -1, true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
}

TEST_F(IntervalMapTest, InsertOnBorderInBetweenSameValue){
    validate_intervals({
        {IntervalOp::Insert, 10, 15, range2val(10, 30), false},
        {IntervalOp::Insert, 20, 30, range2val(10, 30), false},
        {IntervalOp::Insert, 50, 60, range2val(50, 90), false},
        {IntervalOp::Insert, 80, 90, range2val(50, 90), false},
    });
    validate_intervals({
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 15, range2val(10, 30), true},
        {IntervalOp::Find, 16, 19, -1, false},
        {IntervalOp::Find, 20, 30, range2val(10, 30), true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, range2val(50, 90), true},
        {IntervalOp::Find, 61, 79, -1, false},
        {IntervalOp::Find, 80, 90, range2val(50, 90), true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
    validate_intervals({ //start
        {IntervalOp::Insert, 16, 19, range2val(10, 30), false},
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, range2val(50, 90), true},
        {IntervalOp::Find, 61, 79, -1, false},
        {IntervalOp::Find, 80, 90, range2val(50, 90), true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
    validate_intervals({ //end
        {IntervalOp::Insert, 61, 79, range2val(50, 90), false},
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 90, -1, true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
    validate_intervals({
        {IntervalOp::Delete, 0, 100, -1, false},
        {IntervalOp::Insert, 10, 15, -1, false},
        {IntervalOp::Insert, 20, 30, range2val(20, 60), false},
        {IntervalOp::Insert, 50, 60, range2val(20, 60), false},
        {IntervalOp::Insert, 80, 90, -1, false},
    });
    validate_intervals({ //middle
        {IntervalOp::Insert, 31, 49, range2val(20, 60), false},
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 15, -1, true},
        {IntervalOp::Find, 16, 19, -1, false},
        {IntervalOp::Find, 20, 60, -1, true},
        {IntervalOp::Find, 61, 79, -1, false},
        {IntervalOp::Find, 80, 90, -1, true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
}

TEST_F(IntervalMapTest, InsertOnOverlappingFromLeftDifferentValue){
    validate_intervals({
        {IntervalOp::Insert, 10, 15, -1, false},
        {IntervalOp::Insert, 20, 30, -1, false},
        {IntervalOp::Insert, 50, 60, -1, false},
        {IntervalOp::Insert, 80, 90, -1, false},
    });
    validate_intervals({
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 15, -1, true},
        {IntervalOp::Find, 16, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 79, -1, false},
        {IntervalOp::Find, 80, 90, -1, true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
    validate_intervals({ //start
        {IntervalOp::Insert, 7, 12, -1, false},
        {IntervalOp::Find, 0, 6, -1, false},
        {IntervalOp::Find, 7, 12, -1, true},
        {IntervalOp::Find, 13, 15, range2val(10, 15), true},
        {IntervalOp::Find, 16, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 79, -1, false},
        {IntervalOp::Find, 80, 90, -1, true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
    validate_intervals({ //middle
        {IntervalOp::Insert, 18, 25, -1, false},
        {IntervalOp::Find, 0, 6, -1, false},
        {IntervalOp::Find, 7, 12, -1, true},
        {IntervalOp::Find, 13, 15, range2val(10, 15), true},
        {IntervalOp::Find, 16, 17, -1, false},
        {IntervalOp::Find, 18, 25, -1, true},
        {IntervalOp::Find, 26, 30, range2val(20, 30), true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 79, -1, false},
        {IntervalOp::Find, 80, 90, -1, true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
    validate_intervals({ //end
        {IntervalOp::Insert, 70, 82, -1, false},
        {IntervalOp::Find, 0, 6, -1, false},
        {IntervalOp::Find, 7, 12, -1, true},
        {IntervalOp::Find, 13, 15, range2val(10, 15), true},
        {IntervalOp::Find, 16, 17, -1, false},
        {IntervalOp::Find, 18, 25, -1, true},
        {IntervalOp::Find, 26, 30, range2val(20, 30), true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 69, -1, false},
        {IntervalOp::Find, 70, 82, -1, true},
        {IntervalOp::Find, 83, 90, range2val(80, 90), true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
}

TEST_F(IntervalMapTest, InsertOnOverlappingFromRightDifferentValue){
    validate_intervals({
        {IntervalOp::Insert, 10, 15, -1, false},
        {IntervalOp::Insert, 20, 30, -1, false},
        {IntervalOp::Insert, 50, 60, -1, false},
        {IntervalOp::Insert, 80, 90, -1, false},
    });
    validate_intervals({
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 15, -1, true},
        {IntervalOp::Find, 16, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 79, -1, false},
        {IntervalOp::Find, 80, 90, -1, true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
    validate_intervals({ //start
        {IntervalOp::Insert, 13, 17, -1, false},
        {IntervalOp::Find, 0, 6, -1, false},
        {IntervalOp::Find, 10, 12, range2val(10, 15), true},
        {IntervalOp::Find, 13, 17, -1, true},
        {IntervalOp::Find, 18, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 79, -1, false},
        {IntervalOp::Find, 80, 90, -1, true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
    validate_intervals({ //middle
        {IntervalOp::Insert, 58, 70, -1, false},
        {IntervalOp::Find, 0, 6, -1, false},
        {IntervalOp::Find, 10, 12, range2val(10, 15), true},
        {IntervalOp::Find, 13, 17, -1, true},
        {IntervalOp::Find, 18, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 57, range2val(50, 60), true},
        {IntervalOp::Find, 58, 70, -1, true},
        {IntervalOp::Find, 71, 79, -1, false},
        {IntervalOp::Find, 80, 90, -1, true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
    validate_intervals({ //end
        {IntervalOp::Insert, 85, 99, -1, false},
        {IntervalOp::Find, 0, 6, -1, false},
        {IntervalOp::Find, 10, 12, range2val(10, 15), true},
        {IntervalOp::Find, 13, 17, -1, true},
        {IntervalOp::Find, 18, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 57, range2val(50, 60), true},
        {IntervalOp::Find, 58, 70, -1, true},
        {IntervalOp::Find, 71, 79, -1, false},
        {IntervalOp::Find, 80, 84, range2val(80, 90), true},
        {IntervalOp::Find, 85, 99, -1, true},
        {IntervalOp::Find, 100, 100, -1, false},
    });
}

TEST_F(IntervalMapTest, InsertOnOverlappingInBetweenDifferentValue){
    validate_intervals({
        {IntervalOp::Insert, 10, 15, -1, false},
        {IntervalOp::Insert, 20, 30, -1, false},
        {IntervalOp::Insert, 50, 60, -1, false},
        {IntervalOp::Insert, 80, 90, -1, false},
    });
    validate_intervals({
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 15, -1, true},
        {IntervalOp::Find, 16, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 79, -1, false},
        {IntervalOp::Find, 80, 90, -1, true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
    validate_intervals({ //start
        {IntervalOp::Insert, 12, 25, -1, false},
        {IntervalOp::Find, 0, 6, -1, false},
        {IntervalOp::Find, 10, 11, range2val(10, 15), true},
        {IntervalOp::Find, 12, 25, -1, true},
        {IntervalOp::Find, 26, 30, range2val(20, 30), true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 79, -1, false},
        {IntervalOp::Find, 80, 90, -1, true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
    validate_intervals({ //end
        {IntervalOp::Insert, 57, 83, -1, false},
        {IntervalOp::Find, 0, 6, -1, false},
        {IntervalOp::Find, 10, 11, range2val(10, 15), true},
        {IntervalOp::Find, 12, 25, -1, true},
        {IntervalOp::Find, 26, 30, range2val(20, 30), true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 56, range2val(50, 60), true},
        {IntervalOp::Find, 57, 83, -1, true},
        {IntervalOp::Find, 84, 90, range2val(80, 90), true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
}

//insert with [10, 15] [30, 50] [80,90] <- [5, 20] [25, 60] [70,95]; [20, 60]
TEST_F(IntervalMapTest, InsertOnOverlappingOutsideDifferentValue){
    validate_intervals({
        {IntervalOp::Insert, 10, 15, -1, false},
        {IntervalOp::Insert, 20, 30, -1, false},
        {IntervalOp::Insert, 50, 60, -1, false},
        {IntervalOp::Insert, 80, 90, -1, false},
    });
    validate_intervals({
        {IntervalOp::Find, 0, 9, -1, false},
        {IntervalOp::Find, 10, 15, -1, true},
        {IntervalOp::Find, 16, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 79, -1, false},
        {IntervalOp::Find, 80, 90, -1, true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
    validate_intervals({ //start
        {IntervalOp::Insert, 5, 17, -1, false},
        {IntervalOp::Find, 0, 4, -1, false},
        {IntervalOp::Find, 5, 17, -1, true},
        {IntervalOp::Find, 18, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 49, -1, false},
        {IntervalOp::Find, 50, 60, -1, true},
        {IntervalOp::Find, 61, 79, -1, false},
        {IntervalOp::Find, 80, 90, -1, true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
    validate_intervals({ //end
        {IntervalOp::Insert, 40, 70, -1, false},
        {IntervalOp::Find, 0, 4, -1, false},
        {IntervalOp::Find, 5, 17, -1, true},
        {IntervalOp::Find, 18, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 39, -1, false},
        {IntervalOp::Find, 40, 70, -1, true},
        {IntervalOp::Find, 71, 79, -1, false},
        {IntervalOp::Find, 80, 90, -1, true},
        {IntervalOp::Find, 91, 100, -1, false},
    });
    validate_intervals({ //middle
        {IntervalOp::Insert, 75, 95, -1, false},
        {IntervalOp::Find, 0, 4, -1, false},
        {IntervalOp::Find, 5, 17, -1, true},
        {IntervalOp::Find, 18, 19, -1, false},
        {IntervalOp::Find, 20, 30, -1, true},
        {IntervalOp::Find, 31, 19, -1, false},
        {IntervalOp::Find, 40, 70, -1, true},
        {IntervalOp::Find, 71, 74, -1, false},
        {IntervalOp::Find, 75, 95, -1, true},
        {IntervalOp::Find, 96, 100, -1, false},
    });
}

//TODO test has()

}
