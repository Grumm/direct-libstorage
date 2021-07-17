#pragma once

#include <algorithm>
#include <numeric>
#include <map>

#include <TradingAlgo.hpp>

namespace ta{

using MetaSequencePoint = std::pair<Tick, std::vector<Tick>>;
using MetaSequence = std::vector<MetaSequencePoint>;

std::ostream & operator<< (std::ostream &out, const MetaSequencePoint &v){
    out << std::fixed << std::setprecision(2);
    for(auto p: v){
        out << p.first << ": " << p.second << std::endl;
    }
    out << std::endl;

    return out;
}

class SequenceCore{
    const TicksSequence &seq;
public:

    std::vector<std::pair<float, WT>> average_weighted_window(
        const std::vector<std::pair<float, WT>> &points,
            size_t window, size_t step, bool is_weighted = false){
        std::vector<std::pair<float, WT>> newpoints;

        for(size_t i = 0; i < points.size(); i += step){
            float sum = 0;
            uint64_t weight = 0;
            for(size_t j = 0; j < window && (i + window < points.size()); j++){
                auto [price, volume] = points[i + j];

                if(is_weighted)
                    sum += volume * price;
                else
                    sum += price;
                weight += volume;
            }
            if(weight != 0)
                newpoints.push_back(std::make_pair(sum / (is_weighted? weight : window),
                                        ((WT) weight / window) + 1));//weight
        }

        return newpoints;
    }

    std::vector<std::pair<float, WT>>
    get_core_points(MetaSequence points,
            size_t window, size_t step, size_t limit){
        std::vector<std::pair<float, WT>> tmppoints;

        while(flatpoints.size() >= limit){
            tmppoints = flatpoints;
            flatpoints = average_weighted_window(tmppoints, window, step);
            //std::cout << std::fixed << std::setprecision(2) << flatpoints << std::endl;
        }
        return tmppoints;
    }

    /*
        input:TickSequence or MetaSequence
        arguments: range(deafult all), parameters
        output: MetaSequence

        MetaSequence generateCore(const TicksSequence &tseq,
            const std::tuple<size_t, size_t, size_t> &parameters,
            std::pair<size_t, size_t> range);
        MetaSequence generateCore(const MetaSequence &mseq,
            const std::tuple<size_t, size_t, size_t> &parameters,
            std::pair<size_t, size_t> range);
    */

    //size_t window, size_t step, size_t limit_min
    template <bool is_debug = false>
    MetaSequence generateCore(const MetaSequence &mseq,
        const std::tuple<size_t, size_t, size_t> &parameters,
        std::pair<size_t, size_t> range = std::make_pair(0, mseq.size() - 1)){
        auto [window, step, limit] = parameters;

        if(is_debug){
            std::cout << "input:" << std::endl << mseq << std::endl;
        }

        mseq = get_core_points(mseq, parameters, range);

        std::tuple<size_t, size_t, size_t> newparameters{2, 2, limit};
        while(mseq.size() > limit * 2){
            mseq = get_core_points(mseq, newparameters);
        }

        if(is_debug){
            std::cout << "output:" << std::endl << mseq << std::endl;
        }
    }

    template <bool is_debug = false>
    MetaSequence generateCore(const TicksSequence &tseq,
        const std::tuple<size_t, size_t, size_t> &parameters,
        std::pair<size_t, size_t> range){
        MetaSequence mseq;

        for(ssize_t i = range.first; i <= range.second; i++){
            auto tick = tseq.data[i];
            mseq.push_back(std::make_pair(tick, std::vector<Tick>{tick}));
        }

        return generateCore<is_debug>(mseq, parameters);
    }
    /*************************************************/

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


template<class T>
class SequenceInterface{
protected:
    std::vector<T> data;
public:
    size_t size(){ return data.size(); } //final
    virtual T& operator[](size_t idx){ return data[idx]; }

    SequenceInterface(const T &data): data(data) {}

}

/*
Sequence is:
    - sequence of points. type T describes it
*/

/*
MetaSequence is:
    - sequence of points. type T describes it. One point describes sub-Sequence
    - sub-Sequence could be pointed by range in original Sequence, or it could be set of objects
    - T -> container_of_multiple<T> is called transition function
    - transition function could depend on T
    - T should have Constructor() that combines container_of_multiple<T> and creates
        resulting object T, based on different MetaOptions
*/

/*
    T = Tick{
        price, volume, time_period{start, end}
    }
    MetaOptions{
        period{
            time_duration{
                rounded,//starting at 00:00 and every X period
                percise//starting at point A and backwards every period X
            },
            ticks,//every X ticks
            volume, //every X volume
            combined //some combination of 3
        },
        combine{
            avg,
            weighted_avg,
            sum,
            linear_avg//linear coeff and cut in half
            median,
        }
*/

/* 
Sequence adapter:
    - inputs sequence[i] as an y
    - generates x - appropriate for all sequence points
    - passes next pair (x, y)
*/

/* 
Linear coeff algo needs:
    - sequence of data points: (x, y)
*/

};