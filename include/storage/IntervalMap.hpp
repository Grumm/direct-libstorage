#pragma once

#include <map>
#include <tuple>

#include <storage/Utils.hpp>

template<typename T>
concept CMergeMethod1 = requires(T &t1, T &&t2){
    { t1.merge(t2) } -> std::same_as<T &>;
};
template<typename T>
concept CMergeMethod2 = requires(T &t1, T &&t2){
    { T::merge(t1, t2) } -> std::same_as<T &>;
};
template<typename T>
concept CMergeMethod = CMergeMethod1<T> || CMergeMethod2<T>;

template<typename T>
class Merge{
    T &operator()(T &t1, T &&t2);
};

template<CMergeMethod1 T>
class Merge<T>{
    T &operator()(T &t1, T &&t2){
        return t1.merge(std::forward<T>(t2));
    }
};

template<CMergeMethod2 T>
class Merge<T>{
    T &operator()(T &t1, T &&t2){
        return T::merge(t1, std::forward<T>(t2));
    }
};

//what we need: 1) merge for ObjectInstanceRange, 2) size_t in SimpleStorage.hpp

//whole interval has the same value
template<typename K, typename V>
class IntervalMap{
    std::map<K, std::pair<K, V>> m; //[start] -> (end, V)

    auto find_it(const K &k, bool check_value = true) -> decltype(m)::iterator {
        auto it = m.upper_bound(k);
        if(it != m.end()){
            --it;
        } else if(m.end() != m.begin()){
            it = std::prev(m.end());
        }
        if(it != m.end()){
            K iv_start = it->first;
            K iv_end = it->second.first;
            if(!check_value || (k >= iv_start && k <= iv_end))
                return it;
        }
        return m.end();
    }
    auto find_it(const K &k, bool check_value = true) const -> decltype(m)::const_iterator {
        auto it = m.upper_bound(k);
        if(it != m.end()){
            --it;
        } else if(m.end() != m.begin()){
            it = std::prev(m.end());
        }
        if(it != m.end()){
            K iv_start = it->first;
            K iv_end = it->second.first;
            if(!check_value || (k >= iv_start && k <= iv_end))
                return it;
        }
        return m.end();
    }

    //template<typename F> , F &merge, Y &del, K &shrink -> some API
    Result insert_impl(const K &start, const K &end, V &&v){
        auto it = find_it(start, false);
        decltype(it) it_prev = it;
        bool merged = false;
    
        if(it != m.end()){
            auto iv_start = it->first;
            auto iv_end = it->second.first;
            V &val = it->second.second;
            LOG("found something to extend [");

            auto start_prev = start; start_prev--;
            if(iv_end == start_prev && v == val){
                LOG("extending ,%lu -> %lu]", iv_end, end);
                //extend it to [iv_start, end]
                it->second.first = end;
                merged = true;
            }
            it_prev = it;
            it = std::next(it);
        } else {
            //if we cannot find anything, this means closest iter is begin()
            it = m.begin();
        }
        if(it != m.end()){
            auto iv_start = it->first;
            auto iv_end = it->second.first;
            V &val = it->second.second;
            LOG("found something to extend ]");

            auto end_next = end; end_next++;
            if(iv_start == end_next && v == val){
                if(merged){
                    //merge it and it_prev to become [iv_start_prev, iv_end]
                    LOG("merging [%lu, %lu -> %lu]", it_prev->first, it_prev->second.first, iv_end);
                    it_prev->second.first = iv_end;
                    m.erase(it);
                } else {
                    LOG("extending [%lu -> %lu,", iv_start, start);
                    //change key to [start, iv_end]
                    //reinsert element
                    auto nh = m.extract(iv_start);
                    nh.key() = start;
                    m.insert(std::move(nh));
                    merged = true;
                }
            }
        }
        if(!merged){
            auto [it_new, inserted] = m.emplace(start, std::make_pair(end, std::forward<V>(v)));
            LOG_INFO("inserting [%lu, %lu]", start, end);
            ASSERT_ON(!inserted);
        }

        return Result::Success;
    }

    Result del_impl(const K &start, const K &end, size_t iteration = 0){
        auto it = find_it(start, false); //find iter, but start could go after it
        if(it == m.end()){
            it = m.begin();
            if(iteration != 0 || it == m.end()){
                if(!m.empty())
                    return Result::Failure;
                else
                    return Result::Success;
            }
        }

        auto iv_start = it->first;
        auto iv_end = it->second.first;
        V &v = it->second.second;
        auto iv_size = iv_end - iv_start;
        ASSERT_ON(iv_start > iv_end);
        auto it_next = std::next(it);

        if(start > iv_start && start <= iv_end){
            //delete [start, iv_end], keep [iv_start, start)
            it->second.first = start;
            it->second.first--; // start) or start - 1]
            if(end < iv_end){
                //delete [start, end], keep [iv_start, start), (end, iv_end] with duplicated V
                K new_start = end; //(end or [end + 1
                auto [it_new, inserted] = m.emplace(++new_start, std::make_pair(iv_end, v));
                ASSERT_ON(!inserted);
                it = it_new;
            }
        } else if(start <= iv_start && end >= iv_start) {
            if(end < iv_end){
                //delete [iv_start, end], keep (end, iv_end]
                //reinsert element
                auto nh = m.extract(iv_start);
                nh.key() = end;
                nh.key()++; //[end + 1 or (end
                m.insert(std::move(nh));
            } else {
                //delete [iv_start, iv_end]
                m.erase(it);
            }
        } else { //end < iv_start || start > iv_end
            ASSERT_ON(!(end < iv_start || start > iv_end));
            //ASSERT_ON(true);
        }

        if(end > iv_end){
            //N-step range deletion
    
            K iv2_start = it_next->first;
            K iv2_end = it_next->second.first;
            if(iv2_start <= end){
                //recursive
                return del_impl(iv2_start, end, iteration + 1);
            }
        }

        return Result::Success;
    }
public:
    template<typename T>
    class IntervalResult{
        bool is_found;
        T it;
    public:
        IntervalResult(T it): is_found(true), it(it) {}
        IntervalResult(): is_found(false) {}
        bool found() const{
            return is_found;
        }
        std::pair<K, K> range() const{
            ASSERT_ON(!found());
            auto iv_start = it->first;
            auto iv_end = it->second.first;
            return std::make_pair(iv_start, iv_end);
        }
        const V &value() const{
            ASSERT_ON(!found());
            V &v = it->second.second;
            return v;
        }
        V &value() {
            ASSERT_ON(!found());
            V &v = it->second.second;
            return v;
        }
        //merge(IntervalResult other, M m), M - merge<V> object
    };

    //merge<V> -> merge(V&v1_dst, V&&v2), mergeN(V&v1_dst, std::vector<V> &vec)
    //set(): do merge, extend, set
    //extend(IntervalResult) -> IntervalResult
    //get_all_ranges(start, end) -> std::vector<>
    //merge_all_ranges(start, end) -> IntervalResult

    //return whether found, interval start, interval end, V
    auto find(const K &k) const{
        auto it = find_it(k);
        if(it == m.end()){
            return IntervalResult<typename decltype(m)::const_iterator>{};
        }
        return IntervalResult<typename decltype(m)::const_iterator>{it};
    }
    auto find(const K &k) {
        auto it = find_it(k);
        if(it == m.end()){
            return IntervalResult<typename decltype(m)::iterator>{};
        }

        return IntervalResult<typename decltype(m)::iterator>{it};
    }

    Result del(const K &start, const K &end){
        return del_impl(start, end, 0);
    }
/*
    auto find_all_intervals(const K &start, const K &end) -> std::vector<std::pair<K, K>> {
        
    }
    auto find_all_iters(const K &start, const K &end) -> std::vector<std::invoke_result_t<find_it>> {
        
    }
*/

    Result set(const K &start, const K &end, V &&v){
        //find all intervals that have [start, end]
        //TODO
        del(start, end);
        return insert_impl(start, end, std::forward<V>(v));
    }

    Result insert(const K &start, const K &end, V &&v){
        if(has(start, end)){
            del(start, end);
        }
        return insert_impl(start, end, std::forward<V>(v));
    }

    bool has(const K &k) const{
        return find_it(k) != m.end();
    }

    bool has(const K &start, const K &end) const{
        auto it = find_it(end, false);
        if(it == m.end()){
            it = m.begin();
            if(m.empty() || it == m.end())
                return false;
        }
        auto iv_start = it->first;
        auto iv_end = it->second.first;
        return (start <= iv_end && iv_end <=end) || (start <= iv_start && iv_start <= end)
            || (iv_start <= start && start <= iv_end);
    }

    void clear(){
        m.clear();
    }
    //TODO serialize
};
