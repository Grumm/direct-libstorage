#pragma once

#include <algorithm>
#include <numeric>

#include <TradingAlgo.hpp>

#include <boost/math/special_functions/sign.hpp>
namespace bm = boost::math;

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
#if 0
#include <boost/icl/interval_base_set.hpp>
#include <boost/icl/interval_set.hpp>
namespace icl = boost::icl;
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
#endif

template <class WT> class TrendDetector;
using DistanceResults = std::tuple<float, float, float, float, float, float>;
using TrendsContainer = std::vector<std::tuple<size_t, size_t, DistanceResults, std::pair<float, float>>>;

std::ostream & operator<< (std::ostream &out, const DistanceResults &v){
    auto [sum, avg, avg_sq, stdev, max, lin_regr_coeff] = v;

    std::cout << std::setprecision(6) << "(" 
    << sum << "," << avg << "," << avg_sq << "," << stdev << ","
    << max << "," << lin_regr_coeff << ")" << std::setprecision(2);

    return out;
}

std::ostream & operator<< (std::ostream &out, const TrendsContainer &v){
    for(auto e: v){
        auto [i, j, distance, coeff] = e;

        std::cout << "[" << i << "," << j << "] " << coeff.first;
        std::cout << "<-" << distance << std::endl;
    }
    std::cout << std::endl;
    return out;
}

template <class WT = uint64_t>
class TrendDetector{
    const TicksSequence &seq;
public:
    //std::tuple<sum, avg, avg_sq, dev, max, lin_regr_coeff>

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

    std::map<std::pair<float, float>, DistanceResults>
    calulcate_least_distance(const std::vector<std::pair<float, WT>> &points,
            std::pair<size_t, size_t> range, const std::pair<float, float> &lcoeff,
            const std::vector<std::pair<float, float>> &percentiles /* <percent, no more than> */
            ){
        //calculate avg
        //for each point calculate diff between it and avg -> abs()

        std::vector<float> dvect(points.size());
        std::map<std::pair<float, float>, DistanceResults> sigma;
        float a = lcoeff.first;
        float b = lcoeff.second;
        for(size_t i = range.first; i <= range.second; i++){
            /* 
                https://brilliant.org/wiki/dot-product-distance-between-point-and-a-line/
                y = ax + b;
                ax -1y +b = 0
                d(x0, y0) = |ax0 - y0 + b| / sqrt(a*a + 1)
            */
            float x0 = i;
            float y0 = points[i].first;
            float distance = abs(a * x0 - y0 + b) / sqrt(a * a + 1);

            //first only do unweighted TODO
            dvect.push_back(distance);
        }
        std::sort(dvect.begin(), dvect.end(), [](float a, float b){return a < b;});

        for(auto pc: percentiles){
            std::vector<std::pair<float, WT>> distance_points;
            for(size_t i = 0; i < dvect.size(); i++){
                float d = dvect[i];
                //float pcnt = i/dvect.size();
                float nextpcnt = (i + 1)/dvect.size();
                /* check max and ignore last */
                if(d < pc.second && nextpcnt > 1){
                    /* check percentile */
                    auto thispcnt = 1 - pc.first;
                    if(thispcnt > std::numeric_limits<float>::epsilon()){
                        if(thispcnt > nextpcnt){
                            continue;
                        }
                    }
                }

                distance_points.push_back(std::make_pair(d, 1));
            }
            float sum = std::accumulate(distance_points.begin(), distance_points.end(), 0.0, [&]
                (double a, const std::pair<float, WT> &b){
                    return a + b.first;
                });
            float avg = sum / distance_points.size();
            float avg_sq = std::accumulate(distance_points.begin(), distance_points.end(), 0.0, [&]
                (double a, const std::pair<float, WT> &b){
                    return a + (b.first * b.first);
                });
            float stdev = std::sqrt(avg_sq / distance_points.size() - avg * avg);
            auto max = std::max_element(distance_points.begin(), distance_points.end());
            auto lin_regr_coeff = calculate_linear_coeff(distance_points,
                        std::make_pair(0, distance_points.size() - 1));

            sigma[pc] = std::make_tuple(sum, avg, avg_sq, stdev, max->first, lin_regr_coeff.first * 100000);
            //sigma[pc] /= count;
        }

        return sigma;
    }

    /* index [start, end] */
    TrendsContainer get_raw_trends(
            const std::vector<std::pair<float, WT>> &points,
            const DistanceResults &treshold, size_t mintrendpoins = 2){
        TrendsContainer ranges;
        auto [thr_sum, thr_avg, thr_avg_sq, thr_stdev, thr_max, thr_lin_regr_coeff] = treshold;

#define USE_ONLY_PREV_BOUND             1
#define USE_PREV_RANGE                  0
#define USE_PREV_RANGE_NOINTERSECT      0

#if USE_PREV_RANGE
        size_t prev_i = 0;
#endif
#if USE_PREV_RANGE_NOINTERSECT
        std::pair<size_t, size_t> prev_range(0, 0);
#endif
        for(size_t i = 0; i < points.size(); i++){
            //we can go back as deep as pervious i
            for(ssize_t j = points.size(); j >= 0 && j >= (ssize_t)(i + mintrendpoins - 1); j--){
                auto coeff = calculate_linear_coeff(points, std::make_pair(i, j));

                std::pair<float, float> simple_perc(1.00, 0.30);
                auto distancemap = calulcate_least_distance(points,
                    std::make_pair(i, j), coeff, std::vector{simple_perc});
                auto [sum, avg, avg_sq, stdev, max, lin_regr_coeff] = distancemap[simple_perc];
                auto distance = distancemap[simple_perc];

                if(std::get<0>(distance) > std::get<0>(treshold))
                    continue;
                if(std::get<1>(distance) > std::get<1>(treshold))
                    continue;
                if(std::get<2>(distance) > std::get<2>(treshold))
                    continue;
                if(std::get<3>(distance) > std::get<3>(treshold))
                    continue;
                if(std::get<4>(distance) > std::get<4>(treshold))
                    continue;
#if USE_PREV_RANGE_NOINTERSECT
                if((ssize_t)prev_range.second >= j)
                    continue;
#endif
                std::cout << "[" << i << "," << j << "] ";
                std::cout << distancemap[simple_perc] << " ";
#if USE_PREV_RANGE_NOINTERSECT
                prev_range.first = i;
                prev_range.second = j;
#endif
                ranges.push_back(std::make_tuple(i, j, distancemap[simple_perc], coeff));
#if USE_ONLY_PREV_BOUND
                if(j != (ssize_t)i)
                    i = j - 1;
#endif
#if USE_PREV_RANGE
                if(j != (ssize_t)i){
                    auto __tmp = prev_i;
                    prev_i = i--;
                    i = __tmp;
                }
#endif
                break;
            }
#if USE_PREV_RANGE
            prev_i++;
#endif
        }
        std::cout << std::endl;
        //TODO ranges merge by hand with thresh2
        return ranges;
    }

    void print_best(const std::vector<std::pair<float, WT>> &points){
        std::vector<std::pair<size_t, size_t>> best_ranges = {
            {0, 3}, {3, 6},
            {6, 9}, {9, 11},
            {11, 15}, {15, 24},
            {24, 26}, {26, 31},
            {31, 32}, {32, 36},
        };

        for(auto r: best_ranges)
        {
            auto coeff = calculate_linear_coeff(points, std::make_pair(r.first, r.second));

            std::pair<float, float> simple_perc(1.00, 0.30);
            auto distancemap = calulcate_least_distance(points,
                std::make_pair(r.first, r.second), coeff, std::vector{simple_perc});
            auto [sum, avg, avg_sq, stdev, max, lin_regr_coeff] = distancemap[simple_perc];

            std::cout << "[" << r.first << "," << r.second << "] ";
            std::cout << coeff.first << std::setprecision(6) << "<-(" 
            << sum << "," << avg << "," << avg_sq << "," << stdev << ","
            << max << "," << lin_regr_coeff << ") ";
        }
    }

    /* merge with similar coeffitients. if there is a gap, fill it with whatever we calculate */
    TrendsContainer merge_trends(const std::vector<std::pair<float, WT>> &points,
            const TrendsContainer &trends){
        constexpr float flatthreshold0 = 0.015;
        constexpr float flatthreshold1 = 0.020;
        constexpr float similarthreshold = 0.50;
        constexpr size_t skipflatmax = 4;

        TrendsContainer ranges;
        for(size_t i = 0; i < trends.size(); i++){
            auto trend0 = trends[i];
            auto [left0, right0, distance0, coeff0] = trend0;
            for(size_t j = i + 1; j < trends.size(); j++){
                auto trend1 = trends[j];
                auto [left1, right1, distance1, coeff1] = trend1;
                auto coeff2 = coeff1;

                auto acoeff0 = abs(coeff0.first);
                auto acoeff1 = abs(coeff1.first);

                /* ................x1.......x0....0.....x0.......x1.......... */
                /*
                    [x0, 0] - flat
                    [x1, x0] - is similar to flat, so both flat
                    [-inf, x0] - usual behaviour
                */
                if(acoeff1 < flatthreshold0 && acoeff0 < flatthreshold0){
                    //both flat
                    coeff2.first = 0;
                } else if(((acoeff1 < flatthreshold1) && (acoeff0 < flatthreshold0))
                            || ((acoeff0 < flatthreshold1) && (acoeff1 < flatthreshold0))){
                    //still both flat
                    coeff2.first = 0;
                } else if((right0 - left0 + 1 <= skipflatmax && (acoeff0 < flatthreshold0))
                            ||(right1 - left1 + 1 <= skipflatmax && (acoeff1 < flatthreshold0))){
                    /* dont merge with flat less than @skipflatmax elements */
                    break;
                    /* TODO try to lookahead to merge with them */
                } else if(bm::sign(coeff1.first) == bm::sign(coeff0.first)){
                    /* try to decide if they are similar */
                    float rel = 0;
                    if(acoeff0 > acoeff1)
                        rel = 1 - acoeff1 / acoeff0;
                    else
                        rel = 1 - acoeff0 / acoeff1;
                    if(rel < 0)
                        break;

                    if(rel < similarthreshold){
                        coeff2.first = (coeff0.first + coeff1.first) / 2;
                    } else {
                        break;
                    }
                } else{
                    break;
                }
                i++;
                /* TODO calculate coeff via linear regression algo from mered range */
                std::get<1>(trend0) = right0 = right1;
                std::get<3>(trend0) = coeff0 = coeff2;
            }
            ranges.push_back(trend0);
        }

        return ranges;
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

        DistanceResults thresholds = std::make_tuple(0.05, 1.0, 1.0, 1.0, 1.0, 1.0);

        TrendsContainer ranges = get_raw_trends(points, thresholds);
        std::cout << ranges;
        print_best(points);
        ranges = merge_trends(points, ranges);
        std::cout << "Merged:" << std::endl << ranges;
        /* TODO merge trends smart: similar coeffs */

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