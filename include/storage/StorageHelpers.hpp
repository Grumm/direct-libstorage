#pragma once

#include <map>
#include <tuple>
#include <storage/Utils.hpp>
#include <storage/SerializeImpl.hpp>

template<typename T, typename SizeT = size_t>
class SetOrderedBySize{
    std::multimap<SizeT, T> data;
    SetOrderedBySize(std::multimap<SizeT, T> &&data): data(data) {}
public:
    SetOrderedBySize(){}
    void add(SizeT size, const T &t){
        data.insert(std::make_pair(size, t));
    }
    void add(SizeT size, T &&t){
        data.emplace(size, std::forward<T>(t));
    }
    bool has(SizeT size, SizeT atmost = std::numeric_limits<SizeT>::max()) const{
        auto it = data.lower_bound(size);
        if(it == data.end() || it->first > atmost){
            return false;
        }
        return true;
    }
    std::pair<bool, T> get(SizeT size, SizeT atmost = std::numeric_limits<SizeT>::max()){
        auto it = data.lower_bound(size);
        if(it == data.end() || it->first > atmost){
            return std::make_pair(false, T());
        }
        ScopeDestructor sd([&](){ data.erase(it); });
        return std::make_pair(true, it->second);
    }

    
    Result serializeImpl(StorageBuffer<> &buffer) const {
        ASSERT_ON(getSizeImpl() > buffer.allocated());
        size_t offset = 0;

        auto buf = buffer.offset_advance(offset, szeimpl::size(data));
        szeimpl::s(data, buf);

        return Result::Success;
    }
    static SetOrderedBySize<T> deserializeImpl(const StorageBufferRO<> &buffer) {
        size_t offset = 0;
        auto buf = buffer;
        auto data = szeimpl::d<std::multimap<SizeT, T>>(buf);
        
        return SetOrderedBySize{std::move(data)};
    }
    size_t getSizeImpl() const {
        return szeimpl::size(data);
    }
};
