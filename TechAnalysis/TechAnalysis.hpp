#pragma once

#include <TradingAlgo.hpp>

namespace ta{
#if 0
class TradingAnalysisFabric{
    const TicksSequence &seq;
    std::map<std::string, TradingAnalysisAlgo> algos;
public:
    TradingAnalysisFabric(const TicksSequence &seq): seq(seq){}
}
#endif
/* 
We need  algos:
    1) detect trend
        - flat
        - up/down with steepness degree
        - within time range find different trend regions(with varying t.)
    considering
        - price range
        - timeframe

    2) detect volume spikes
        - summarise over few small lots
        - detect slow growth of volume over time in huge lots

    3) detect volatility
        - at current time range
        - within time range find and separate volatility regions(with different v.)

    4) range detector
        - detects ranges with different parameters, such as 
            volatility, trend, volume, price levels

*/

template<class T, class U>
std::ostream & operator<< (std::ostream &out, const std::vector<T, U> &v){
    for (auto o: v){
        out << "(" << o.first << " " << o.second << ") ";
    }
    return out;
}

class TrendDetector{
    const TicksSequence &seq;
public:
    enum trend_type{
        trend_flat,
        trend_up,
        trend_down,
    };
    struct trend{
        enum trend_type direction;
        float steepness; //angle

        bt::time_duration duration;
        size_t num_ticks;
        size_t window;
    };

    TrendDetector(const TicksSequence &seq):
        seq(seq){}

    std::vector<std::pair<float, uint64_t>> average_weighted_window(
        const std::vector<std::pair<float, uint64_t>> &points,
            size_t window, size_t step = 1){
        std::vector<std::pair<float, uint64_t>> newpoints;

        for(size_t i = 0; i < points.size(); i += step){
            float sum = 0;
            uint64_t weight = 0;
            for(size_t j = 0; j < window && (i + window < points.size()); j++){
                auto [price, volume] = points[i + j];

                sum += volume * price;
                weight += volume;
            }
            if(weight != 0)
                newpoints.push_back(std::make_pair(sum / weight, (weight / window) + 1));//weight
        }

        return newpoints;
    }

    void get_approximaiton_function(const std::vector<std::pair<float, uint64_t>> &points){

    }

    std::vector<std::pair<float, uint64_t>>
    get_core_points(std::vector<std::pair<float, uint64_t>> flatpoints,
            size_t window, size_t step){
        std::vector<std::pair<float, uint64_t>> tmppoints;

        while(flatpoints.size() >= 2){
            tmppoints = flatpoints;
            flatpoints = average_weighted_window(tmppoints, window, step);
            std::cout << std::fixed << std::setprecision(2) << flatpoints << std::endl;
        }
        return tmppoints;
    }

    trend detect(size_t index, size_t offset, size_t window, size_t step = 1){
        trend trend;
        trend.direction = trend_flat;

        std::vector<std::pair<float, uint64_t>> flatpoints;
        for(ssize_t i = index; i >= 0 && i >= (ssize_t)(index - offset + window); i--){
            auto tick = seq.data[i];
            flatpoints.push_back(std::make_pair(tick.price, tick.volume));
        }
        std::cout << "input:" << std::endl;
        std::cout << std::fixed << std::setprecision(2) << flatpoints << std::endl;

        flatpoints = get_core_points(flatpoints, window, step);
        std::cout << "<-final" << std::endl;
        flatpoints = get_core_points(flatpoints, 3, 1);
        std::cout << "polished:" << std::endl;
        std::cout << std::fixed << std::setprecision(2) << getp(flatpoints, 3, 1) << std::endl;
        //have X(X = window * 2 -1) points with values
        return trend;
    }

    trend detect(bt::ptime at, bt::time_duration duration, size_t window, size_t step = 1){
        size_t index = 0;
        bool found = false;

        for(size_t i = 0; i < seq.data.size(); i++){
            auto tick = seq.data[i];
            if(tick.time > at){
                index = i;
                found = true;
                break;
            }
        }
        if(!found)
            throw std::invalid_argument("not found time in seq");

        auto tick0 = seq.data[index];

        for(size_t offset = 1; offset <= index; offset++){
            auto i = index - offset;
            auto tick = seq.data[i];
            if(!bt::time_period(tick.time, duration).contains(tick0.time))
                return detect(index, offset - 1, window, step);
        }

        return detect(index, index, window, step);
    }
#if 0
    trend detect(bt::ptime at, bt::time_duration duration, bt::time_duration window){
        //TODO
        //return detect(index, offset, window);
    }
#endif

};

};
