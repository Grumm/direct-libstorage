#pragma once

#include <map>
#include <tuple>
#include <ranges>

#include <storage/Utils.hpp>
#include <storage/StorageUtils.hpp>
#include <storage/SerializeImpl.hpp>

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
    using M_TYPE = decltype(m);

    IntervalMap(std::map<K, std::pair<K, V>> &&m): m(m) {}

    template<typename T>
    T next(const T &t){
        T new_t = t;
        return ++new_t;
    }
    template<typename T>
    T prev(const T &t){
        T new_t = t;
        return --new_t;
    }
    auto emplace(const K &start, const K &end, const V &val) -> std::ranges::iterator_t<M_TYPE> {
        auto [it, inserted] = m.emplace(start, std::make_pair(end, val));
        LOG_INFO("inserting [%lu, %lu]", start, end);
        ASSERT_ON(!inserted);
        return it;
    }

    //TODO rework it to return Interval Result
    auto find_it(const K &k, bool check_value = true) -> std::ranges::iterator_t<M_TYPE> {
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
    auto find_it(const K &k, bool check_value = true) const -> M_TYPE::const_iterator {
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
    Result insert_impl(const K &start, const K &end, const V &v){
        auto it = find_it(start, false);
        decltype(it) it_prev = it;
        bool merged = false;
    
        if(it != m.end()){
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
            emplace(start, end, v);
        }

        return Result::Success;
    }

    //template <typename F> , F &f
    Result del_impl(const K &start, const K &end, size_t iteration){
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
        auto iv_result = IntervalResult{it};

        auto [iv_start, iv_end] = iv_result.range();
        auto v = iv_result.value();
        //auto iv_size = iv_end - iv_start;
        ASSERT_ON(iv_start > iv_end);
        auto it_next = std::next(it);

        if(start > iv_start && start <= iv_end){
            //delete [start, iv_end], keep [iv_start, start)
            iv_result.end() = prev(start); // start) or start - 1]
            if(end < iv_end){
                //delete [start, end], keep [iv_start, start), (end, iv_end] with duplicated V
                it = emplace(next(end), iv_end, v); //(end or [end + 1
            }
        } else if(start <= iv_start && end >= iv_start) {
            if(end < iv_end){
                //delete [iv_start, end], keep (end, iv_end]
                //reinsert element
                iv_result.updateStart(m, next(end)); //[end + 1 or (end
            } else {
                //delete [iv_start, iv_end]
                m.erase(it);
            }
        } else { //end < iv_start || start > iv_end
            ASSERT_ON(!(end < iv_start || start > iv_end));
            //ASSERT_ON(true);
        }

        if(end > iv_end && it_next != m.end()){
            //N-step range deletion
            iv_result = IntervalResult{it_next};
            auto [iv2_start, iv2_end] = iv_result.range();
            ASSERT_ON(iv2_start <= iv_start);
            if(iv2_start <= end){
                //recursive
                //TODO rework to do for_each_interval()
                return del_impl(iv2_start, end, iteration + 1);
            }
        }

        return Result::Success;
    }
public:
    template<typename T>
    class IntervalResultImpl{
        bool is_found;
        T it;
    protected:
        friend class IntervalMap;
        T &iter(){ return it; }
        //shall only be used when all checks verified
        K &end(){ return it->second.first; }
        //invalidated iter()
        bool updateStart(M_TYPE &m, K &&new_start){
            auto nh = m.extract(it);
            nh.key() = new_start;
            auto insert_ret = m.insert(std::move(nh));
            it = insert_ret.position;
            if (it == m.end() || !insert_ret.inserted){
                is_found = false;
            }
            return is_found;
        }
    public:
        IntervalResultImpl(T it): is_found(true), it(it) {}
        IntervalResultImpl(): is_found(false) {}
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
    using ConstIntervalResult = IntervalResultImpl<typename M_TYPE::const_iterator>;
    using IntervalResult = IntervalResultImpl<std::ranges::iterator_t<M_TYPE>>;
    IntervalMap(){}

    //merge<V> -> merge(V&v1_dst, V&&v2), mergeN(V&v1_dst, std::vector<V> &vec)
    //set(): do merge, extend, set
    //extend(IntervalResult) -> IntervalResult
    //get_all_ranges(start, end) -> std::vector<>
    //merge_all_ranges(start, end) -> IntervalResult

    //return whether found, interval start, interval end, V
    auto find(const K &k) const -> ConstIntervalResult{
        auto it = find_it(k);
        if(it == m.end()){
            return ConstIntervalResult{};
        }
        return ConstIntervalResult{it};
    }
    auto find(const K &k) -> IntervalResult{
        auto it = find_it(k);
        if(it == m.end()){
            return IntervalResult{};
        }

        return IntervalResult{it};
    }

    Result del(const K &start, const K &end){
        return del_impl(start, end, 0);
    }
    template<typename F>
    void for_each_interval(const K &start, const K &end, F &&f) {
        auto it = find_it(start);
        while(it != m.end()){
            auto iv_result = IntervalResult{it};
            auto [iv_start, iv_end] = iv_result.range();
            if (end < iv_start){
                break;
            }
            f(std::move(iv_result), std::max(iv_start, start), std::min(iv_end, end), iv_result.value());
            it++;
        }
    }
/*
    template<typename F>
    void del_foreach(const K &start, const K &end, F &&f) {

    }
    auto find_all_iters(const K &start, const K &end) -> std::vector<IntervalResult> {
        
    }
*/

    Result set(const K &start, const K &end, const V &v){
        //find all intervals that have [start, end]
        //TODO
        del(start, end);
        return insert_impl(start, end, v);
    }

    Result insert(const K &start, const K &end, const V &v){
        if(has(start, end)){
            del(start, end);
        }
        return insert_impl(start, end, v);
    }

    bool has(const K &k) const{
        return find_it(k) != m.end();
    }
    //TODO unit test
    //TODO only expanding end, need expanding start as well
    template<typename F>
    Result expand(const K &start, const K &new_end, F &&f){
        //find range with start, ensure it is [<start, new_end)
        auto it = find_it(start);
        if(it == m.end()){
            return Result::Failure;
        }
        auto iv_result = IntervalResult{it};
        auto [iv_start, iv_end] = iv_result.range();
        ASSERT_ON(iv_start > start); //TODO can be implemented as well
        ASSERT_ON(new_end < start);
        if (iv_end >= new_end){
            //no need to expand, shrink
            K old_end = iv_end;
            iv_result.end() = new_end;
            f(iv_result, old_end, new_end);
            return Result::Success;
        }
        //expand range if can do it, return false if fails
        auto newit = it; newit++;
        if (newit != m.end()){
            //has next range
            auto next_iv_result = IntervalResult{newit};
            auto [n_iv_start, n_iv_end] = next_iv_result.range();
            if(n_iv_start <= new_end){
                //overlaps next range, failing
                return Result::Failure;
            }
            //okay, continue updating range and value
        }
        K old_end = iv_end;
        iv_result.value() += new_end - iv_end;
        iv_result.end() = new_end;
        f(iv_result, old_end, new_end);

        return Result::Success;
    }

    //TODO unit test
    template<typename F>
    Result split(IntervalResult &iv_result, const K &mid_left, F &&f){
        auto [iv_start, iv_end] = iv_result.range();
        ASSERT_ON(mid_left >= iv_end); //or maybe RET_ON_FAIL()
        ASSERT_ON(iv_start > mid_left);
        
        auto mid_right = mid_left; mid_right++;
        auto val_right = f(iv_result.value(), iv_start, mid_left, mid_right, iv_end);
        iv_result.end() = mid_left;

        emplace(mid_right, iv_end, val_right);

        return Result::Success;
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

    Result serializeImpl(StorageBuffer<> &buffer) const {
        ASSERT_ON(getSizeImpl() > buffer.allocated());
        size_t offset = 0;
        return SerializeSequentially(buffer, offset, m);
    }
    static IntervalMap<K, V> deserializeImpl(const StorageBufferRO<> &buffer) {
        auto buf = buffer;
        size_t offset = 0;
        auto [m] = DeserializeSequentially<std::map<K, std::pair<K, V>>>(buf, offset);
        
        return IntervalMap{std::move(m)};
    }
    size_t getSizeImpl() const {
        return szeimpl::size(m);
    }
};
