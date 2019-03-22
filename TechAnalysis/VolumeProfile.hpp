#pragma once

#include <algorithm>
#include <numeric>
#include <map>

#include <TradingAlgo.hpp>

namespace ta{

using VolumeMap = std::map<float, uint64_t>;

std::ostream & operator<< (std::ostream &out, const VolumeMap &v){
    out << std::fixed << std::setprecision(2);
    for(auto p: v){
        out << p.first << ": " << p.second << std::endl;
    }
    out << std::endl;

    return out;
}

class VolumeProfile{
    const TicksSequence &seq;
public:

    VolumeProfile(const TicksSequence &seq):
        seq(seq){}

    void VolumeMapAdd(VolumeMap &vmap, const std::pair<float, uint64_t> &v){
        if(vmap.find(v.first) != vmap.end()){
            vmap[v.first] += v.second;
        } else {
            vmap[v.first] = v.second;
        }
    }

    VolumeMap detect(size_t index, size_t offset){
        VolumeMap vmap;

        for(ssize_t i = index; i >= 0 && i >= (ssize_t)(index - offset); i--){
            auto tick = seq.data[i];
            VolumeMapAdd(vmap, std::make_pair(tick.price, tick.volume));
        }
        return vmap;
    }

    VolumeMap detect(bt::ptime at_start, bt::ptime at_end){
        size_t index = 0;
        bool found = false;

        for(size_t i = 0; i < seq.data.size(); i++){
            auto tick = seq.data[i];
            if(tick.time > at_end){
                index = i;
                found = true;
                break;
            }
        }
        if(!found)
            throw std::invalid_argument("not found time in seq");

        auto tick0 = seq.data[index];
        auto tperiod = bt::time_period(at_start, at_end);

        for(size_t offset = 1; offset <= index; offset++){
            auto i = index - offset;
            auto tick = seq.data[i];
            if(!tperiod.contains(tick.time))
                return detect(index, offset - 1);
        }

        return detect(index, index);
    }

    /* end, period backwards */
    VolumeMap detect(bt::ptime at_end, bt::time_duration duration){
        bt::ptime at_start = at_end - duration;
        return detect(at_start, at_end);
    }
};

};