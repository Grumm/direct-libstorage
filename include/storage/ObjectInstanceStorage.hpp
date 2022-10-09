#pragma once

#include <deque>
#include <vector>
#include <map>

#include <storage/StorageUtils.hpp>
#include <storage/IntervalMap.hpp>
#include <storage/SerializeImpl.hpp>

/*
ObjectStorage = Object Sequence Storage
Stores multiple objects.
Two different implementations:
* Simple, vector-style, K^2n allocation, bit-offset addressing etc.
* Complex, Object sizes varies
*/


//where to keep all the instances
//two pools: one we allocate the range, another is where we allocate single instance
template<typename T, AllocationPattern A = AllocationPattern::Default, size_t MAX_OBJS = std::numeric_limits<size_t>::max()>
//requires CSerializableFixedSize<T>
class ObjectInstanceStorage{
    class ObjectInstanceRange{
        size_t range_start;
        size_t size;
        std::deque<T> instance_vector;
        std::vector<bool> deleted;
    public:
        bool operator==(const ObjectInstanceRange &other) const {
            return range_start == other.range_start && size == other.size;
        }
        ObjectInstanceRange(size_t range_start, size_t range_end):
                            range_start(range_start), size(range_end - range_start + 1) {}
        bool has(size_t index) const{
            return (index < size) && (deleted.size() <= index || !deleted[index]);
        }
        void del(size_t index){
            del_range(index, index);
        }
        void del_range(size_t start, size_t end){
            ASSERT_ON(start > end);
            ASSERT_ON(end >= size);
            if(end == size - 1){
                instance_vector.erase(instance_vector.begin() + start, instance_vector.end());
            }
            if(deleted.size() <= end){
                deleted.resize(end + 1, false);
            }
            for(size_t i = start; i <= end; i++){
                deleted[i] = true;
                auto tmpobj{std::move(instance_vector[i])};
            }
        }
        void add(size_t index, T &&t){
            auto ret_lambda = [t = std::forward<T>(t)](size_t, size_t){ return t; };
            add_range(index, index, ret_lambda);
        }
        template<typename F>
        void add(size_t index, F &f){
            add_range(index, index, f);
        }
        template<typename F>
        void add_range(size_t start, size_t end, F &f){
            ASSERT_ON(start > end);
            ASSERT_ON(end >= size);
            if(deleted.size() <= end){
                deleted.resize(end + 1, false);
            }
            //size=0, [2, 3]: [0],[1]; [2],[3]
            //size=0, [0, 2]: tmp_end=0; [0],[1],[2]; end=0;
            //size=0, [0, 3]: tmp_end=0; [0],[1],[2],[3];
            //size=1, [0, 3]: tmp_end=1; [1],[2],[3]; end=0
            if(instance_vector.size() < start){
                //if we haven't allocated up to this range yet, do it in one go
                for(size_t i = instance_vector.size(); i < start; i++){
                    instance_vector.emplace_back(f(range_start, i));
                    deleted[i] = false;
                }
            } else {
                if(instance_vector.size() < end - 1) {
                    //fill the tail
                    auto tmp_end = instance_vector.size();
                    for(size_t i = tmp_end; i <= end; i++){
                        instance_vector.emplace_back(f(range_start, i));
                        deleted[i] = false;
                    }
                    if(tmp_end == 0)
                        return;
                    end = tmp_end - 1; //what if start = tmp_end = 0, end = 1 => expected 0 done after
                }
            }
            for(size_t i = start; i <= end; i++){
                instance_vector[i] == std::move(f(range_start, i));
                deleted[i] = false;
            }
        }
        std::pair<bool, T*> get(size_t index){
            if(has(index)){
                return std::make_pair(true, &instance_vector[index]);
            } else {
                return std::make_pair(false, nullptr);
            }
        }
        void clear(){
            instance_vector.clear();
            deleted.clear();
        }
    };
    std::map<size_t, T> instance_map; //for single operations
    IntervalMap<size_t, ObjectInstanceRange> instance_range; //for ranges operations: [start, end] -> (dequeue, size)
    static constexpr size_t RANGE_THRESHOLD = 8;
public:
    ObjectInstanceStorage(){}
    template<typename F>
    void add_range(size_t start, size_t end, F &f){
        auto size = end - start + 1;
        if(instance_range.has(start, end)){
            //TODO for all ranges in between, extend all current ranges and merge them into one
            //handle simple case where we have only one 
            //TODO visitor to merge ranges together
            //extend existing range
            ASSERT_ON(true);
        } else if(size < RANGE_THRESHOLD){
            for(size_t i = start; i <= end; i++){
                auto [it, inserted] = instance_map.emplace(i, f(start, i - start));
                ASSERT_ON(!inserted);
            }
        } else {
            ObjectInstanceRange ir{start, end};
            ir.add_range(0, size - 1, f);
            instance_range.insert(start, end, std::move(ir));
        }
    }
    void add(size_t index, T &&t){
        auto iv_result = instance_range.find(index);
        if(iv_result.found()){
            auto &v = iv_result.value();
            auto [iv_start, iv_end] = iv_result.range();
            size_t iv_index = index - iv_start;
            v.add(iv_index, std::forward<T>(t));
        } else {
            auto [it, inserted] = instance_map.emplace(index, std::forward<T>(t));
            ASSERT_ON(!inserted);
        }
    }
    void del(size_t index){
        del_range(index, index);
    }
    void del_range(size_t start, size_t end){
        for(size_t i = start; i <= end; i++){
            auto iv_result = instance_range.find(i);
            if(iv_result.found()){
                auto &v = iv_result.value();
                auto [iv_start, iv_end] = iv_result.range();
                size_t iv_index = i - iv_start;
                v.del_range(iv_index, std::min(end, iv_end) - iv_start);
                i = iv_end; //skip found range
            }
            auto it = instance_map.find(i);
            if(it != instance_map.end()){
                instance_map.erase(it);
            }
        }
    }
    std::pair<bool, T *> get(size_t index) {
        auto it = instance_map.find(index);
        if(it != instance_map.end()){
            return std::make_pair(true, &it->second);
        }
        auto iv_result = instance_range.find(index);
        if(iv_result.found()){
            auto &v = iv_result.value();
            auto [iv_start, iv_end] = iv_result.range();
            size_t iv_index = index - iv_start;
            return v.get(iv_index);
        }
        return std::make_pair(false, nullptr);
    }
    bool has(size_t index) {
        auto it = instance_map.find(index);
        if(it != instance_map.end()){
            return true;
        }
        auto iv_result = instance_range.find(index);
        if (iv_result.found()) {
            auto &v = iv_result.value();
            auto [iv_start, iv_end] = iv_result.range();
            size_t iv_index = index - iv_start;
            return v.has(iv_index);
        }
        return false;
    }
};

/*
ObjectInstanceStorage can have multiple implementations. Idea: make it an interface

Purpose:
* to have ability to control lifetime and allocation of objects
* to be able to keep objects in memory and in the storage
* provide control over the objects to the user

Key features:
* add object at index - although do we need this feature, this could be a constraint
* add mutiple object within index range
* add multiple objects at the end of storage
* del objects within index range
* get std::array of objects

Performance features
* minimalistic implementation to get/add object - const time for item in range
* serialization/deserialization
* 
*/