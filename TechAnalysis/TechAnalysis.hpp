#pragma once

#include <algorithm>

#include <TradingAlgo.hpp>

#include <boost/icl/interval_base_set.hpp>
#include <boost/icl/interval_set.hpp>
namespace icl = boost::icl;

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
        //out << "(" << o.first << " " << o.second << ") ";
        out << o.first << '\t' << o.second << std::endl;
    }
    return out;
}

template<class T>
std::ostream & operator<< (std::ostream &out, const icl::interval_set<T> &v){
    for (auto it = v.begin(); it != v.end(); it++){
        if(it->lower() == it->upper())
            std::cout << "[" << it->lower() << "]" << std::endl;
        else
            std::cout << "[" << it->lower() << "," << it->upper() << "]" << std::endl;
    }
    return out;
}

template <class WT = uint64_t>
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

    float calulcate_least_distance(const std::vector<std::pair<float, WT>> &points,
            std::pair<size_t, size_t> range){
        //calculate avg
        //for each point calculate diff between it and avg -> abs()

        float sum = 0;
        for(size_t i = range.first; i <= range.second; i++){
            sum += points[i].first;//first only do unweighted TODO
        }
        float avg = sum / (range.second - range.first + 1);

        float diff = 0;
        for(size_t i = range.first; i <= range.second; i++){
            //diff += abs(points[i].first - avg);
            diff += pow(points[i].first - avg, 2);
        }

        return diff / (range.second - range.first + 1) * 100;
    }

    /* https://prog-cpp.ru/mnk/ */
    std::pair<float, float>
    calculate_linear_coeff(const std::vector<std::pair<float, WT>> &points,
            std::pair<size_t, size_t> range){
        //calculate avg
        //for each point calculate diff between it and avg -> abs()

        float sumx = 0;
        float sumx2 = 0;
        float sumy = 0;
        float sumxy = 0;

        for(size_t i = range.first; i <= range.second; i++){
            float y = points[i].first;//first only do unweighted TODO
            float x = i;
            sumx += x;
            sumx2 += x*x;
            sumy += y;
            sumxy += x*y;
        }

        float n = range.second - range.first + 1;
        float a = (n*sumxy - sumx * sumy) / (n * sumx2 - sumx * sumx);
        float b = (sumy - a * sumx) / n;

        return std::make_pair(a, b);
    }

    /* index (start, end) */
    icl::interval_set<size_t> get_flat_trends(
            const std::vector<std::pair<float, WT>> &points){
        constexpr float flatthresh = 0.03;
        constexpr float flatthresh2 = 0.09; //for distance after merge
        constexpr size_t mintrendpoins = 3;
        icl::interval_set<size_t> flats;

        for(size_t i = 0; i < points.size(); i++){
            for(size_t j = points.size(); j >= i + mintrendpoins; j--){
                auto coeff = calculate_linear_coeff(points, std::make_pair(i, j));

                float distance = calulcate_least_distance(points, std::make_pair(i, j));
                if(distance > flatthresh)
                    continue;
                std::cout << "[" << i << "," << j << "]";
                std::cout << distance << " ";
                //i = j;
                flats.insert(icl::interval_set<size_t>::interval_type::open(i, j));
                break;
            }
        }
        //TODO ranges merge by hand with thresh2
        return flats;
    }
    /*
    algos: lin approx, flat 
        trend detection:
            1) detect flat trends: with flatness criteria
            2) between flat trends run lin approx algo
    */
    std::vector<trend> get_trends(
            const std::vector<std::pair<float, WT>> &points){
        std::vector<trend> trends;
        trend t;
        t.direction = trend_flat;
        trends.push_back(t);
        auto ranges = get_flat_trends(points);
        std::cout << ranges;

        return trends;
    }

    std::vector<std::pair<float, WT>>
    get_core_points(std::vector<std::pair<float, WT>> flatpoints,
            size_t window, size_t step, size_t limit){
        std::vector<std::pair<float, WT>> tmppoints;

        while(flatpoints.size() >= limit){
            tmppoints = flatpoints;
            flatpoints = average_weighted_window(tmppoints, window, step);
            //std::cout << std::fixed << std::setprecision(2) << flatpoints << std::endl;
        }
        return tmppoints;
    }

    std::vector<trend> detect(size_t index, size_t offset,
            size_t window, size_t step, size_t limit){
        std::vector<std::pair<float, WT>> flatpoints;

        for(ssize_t i = index; i >= 0 && i >= (ssize_t)(index - offset + window); i--){
            auto tick = seq.data[i];
            flatpoints.push_back(std::make_pair(tick.price, tick.volume));
        }
        //std::cout << "input:" << std::endl;
        //std::cout << std::fixed << std::setprecision(2) << flatpoints << std::endl;

        flatpoints = get_core_points(flatpoints, window, step, limit);
        //std::cout << "polished:" << std::endl;
        while(flatpoints.size() > limit * 2){
            flatpoints = get_core_points(flatpoints, 2, 2, limit);
        }
        std::cout << "final:" << std::endl;
        std::reverse(flatpoints.begin(), flatpoints.end());
        std::cout << std::fixed << std::setprecision(2) << flatpoints << std::endl;
        //have X(X = window * 2 -1) points with values
        return get_trends(flatpoints);
    }

    std::vector<trend> detect(bt::ptime at, bt::time_duration duration,
            size_t window, size_t step, size_t limit){
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
                return detect(index, offset - 1, window, step, limit);
        }

        return detect(index, index, window, step, limit);
    }
#if 0
    trend detect(bt::ptime at, bt::time_duration duration, bt::time_duration window){
        //TODO
        //return detect(index, offset, window);
    }
#endif

};

};


/*
определение диапазонов: торговли, покупки, продажи
определение дельты(sum(bid) - sum(ask)) в любой момент времени скользящее и корреляция
    с прайсом в ближайшее время
*/