#pragma once

#include <algorithm>
#include <numeric>

#include <TradingAlgo.hpp>

#include <boost/math/special_functions/sign.hpp>
namespace bm = boost::math;

namespace ta{

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

    out << std::setprecision(6) << "(" 
    << sum << "," << avg << "," << avg_sq << "," << stdev << ","
    << max << "," << lin_regr_coeff << ")" << std::setprecision(2);

    return out;
}

std::ostream & operator<< (std::ostream &out, const TrendsContainer &v){
    for(auto e: v){
        auto [i, j, distance, coeff] = e;

        out << "[" << i << "," << j << "] " << coeff.first;
        out << "<-" << distance << std::endl;
    }
    out << std::endl;
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
        
    /* index [start, end] */
    template <bool is_debug = false>
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
                auto newcoeff = get_linear_regr(std::make_pair(i, j), points);
                auto distance = newcoeff.second;
                auto coeff = newcoeff.first;

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
                if(is_debug){
	                std::cout << "[" << i << "," << j << "] ";
	                std::cout << distance << " ";
	            }
#if USE_PREV_RANGE_NOINTERSECT
                prev_range.first = i;
                prev_range.second = j;
#endif
                ranges.push_back(std::make_tuple(i, j, distance, coeff));
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
            std::pair<float, float> full_perc = std::make_pair(1.0, 0.3);
            auto newcoeff = get_linear_regr(r, points, full_perc);
            auto distance = newcoeff.second;
            auto coeff = newcoeff.first;
            auto [sum, avg, avg_sq, stdev, max, lin_regr_coeff] = distance;

            std::cout << "[" << r.first << "," << r.second << "] ";
            std::cout << coeff.first << std::setprecision(6) << "<-(" 
            << sum << "," << avg << "," << avg_sq << "," << stdev << ","
            << max << "," << lin_regr_coeff << ") ";
        }
    }

    /* merge with similar coeffitients. if there is a gap, fill it with whatever we calculate */
    template <bool is_debug = false>
    TrendsContainer merge_trends_basic(const std::vector<std::pair<float, WT>> &points,
            const TrendsContainer &trends,
            const std::tuple<float, float, float, size_t> &thresholds){
        auto [flatthreshold0, flatthreshold1, similarthreshold, skipflatmax] = thresholds;

        TrendsContainer ranges;
        for(size_t i = 0; i < trends.size(); i++){
            auto trend0 = trends[i];
            auto [left0, right0, distance0, coeff0] = trend0;
            for(size_t j = i + 1; j < trends.size(); j++){
                auto trend1 = trends[j];
                auto [left1, right1, distance1, coeff1] = trend1;

                auto acoeff0 = abs(coeff0.first);
                auto acoeff1 = abs(coeff1.first);

                /* ................x1.......x0....0.....x0.......x1.......... */
                /*
                    [x0, 0] - flat
                    [x1, x0] - is similar to flat, so both flat
                    [-inf, x0] - usual behaviour
                */
                if(is_debug){
                    std::cout << "basic [" << left0 << ", " << right0 << "] <-";
                    std::cout << "[" << left1 << ", " << right1 << "]<-?????? ";
                    std::cout << coeff0.first << " " << coeff1.first;
                    std::cout << std::endl;
                }
                if(acoeff1 <= flatthreshold0 && acoeff0 <= flatthreshold0){
                    //both flat
                    std::get<3>(trend0).first = 0;
                } else if(((acoeff1 <= flatthreshold1) && (acoeff0 <= flatthreshold0))
                            || ((acoeff0 <= flatthreshold1) && (acoeff1 <= flatthreshold0))){
                    //still both flat
                    std::get<3>(trend0).first = 0;
                } else if((right0 - left0 + 1 <= skipflatmax && (acoeff0 <= flatthreshold0))
                            ||(right1 - left1 + 1 <= skipflatmax && (acoeff1 <= flatthreshold0))){
                    /* dont merge with flat less than @skipflatmax elements */
                    break;
                    /* TODO try to lookahead to merge with them */
                } else if(acoeff1 <= flatthreshold0 || acoeff0 <= flatthreshold0){
                    break;
                } else if(bm::sign(coeff1.first) == bm::sign(coeff0.first)){
                    /* try to decide if they are similar */
                    float rel;
                    if(acoeff0 > acoeff1)
                        rel = 1 - acoeff1 / acoeff0;
                    else
                        rel = 1 - acoeff0 / acoeff1;
                    if(rel < 0){
                        if(is_debug){
                            std::cout << "basic [" << left0 << ", " << right0 << "] <-";
                            std::cout << "[" << left1 << ", " << right1 << "]<-";
                            std::cout << rel << " " << similarthreshold;
                            std::cout << std::endl;
                        }
                        break;
                    }

                    if(rel < similarthreshold){
                        auto newcoeff = get_linear_regr(std::make_pair(left0, right1), points);
                        if(is_debug){
                            std::cout << "basic [" << left0 << ", " << right0 << "] <-";
                            std::cout << "[" << left1 << ", " << right1 << "]<-" << newcoeff.first.first;
                            std::cout << std::endl;
                        }
                        if(newcoeff.first.first <= flatthreshold0){
                            newcoeff.first.first = 0;
                        }
                        std::get<2>(trend0) = distance0 = newcoeff.second;
                        std::get<3>(trend0) = coeff0 = newcoeff.first;
                    } else {
                        if(is_debug){
                            std::cout << "basic [" << left0 << ", " << right0 << "] <-";
                            std::cout << "[" << left1 << ", " << right1 << "]<-";
                            std::cout << rel << " " << similarthreshold;
                            std::cout << std::endl;
                        }
                        break;
                    }
                } else{
                    break;
                }
                i++;
                /* TODO calculate coeff via linear regression algo from mered range */
                std::get<1>(trend0) = right0 = right1;
            }
            ranges.push_back(trend0);
        }

        return ranges;
    }

    template <bool is_debug = false>
    TrendsContainer merge_trends_skipflats(const std::vector<std::pair<float, WT>> &points,
            const TrendsContainer &trends,
            const std::tuple<float, float, float, size_t> &thresholds){
        auto [flatthreshold0, flatthreshold1, similarthreshold, skipflatmax] = thresholds;

        TrendsContainer ranges;
        for(size_t i = 0; i < trends.size(); i++){
            auto trend0 = trends[i];
            auto [left0, right0, distance0, coeff0] = trend0;

            auto acoeff0 = abs(coeff0.first);

            if(acoeff0 > flatthreshold0){
                for(size_t j = i + 1; j < trends.size() - 1; j += 2){
                    auto trend1 = trends[j];
                    auto [left1, right1, distance1, coeff1] = trend1;
                    
                    auto trend2 = trends[j + 1];
                    auto [left2, right2, distance2, coeff2] = trend2;

                    auto acoeff1 = abs(coeff1.first);
                    auto acoeff2 = abs(coeff2.first);
                    if(is_debug){
                        std::cout << "merging [" << left0 << ", " << right0 << "] <-";
                        std::cout << "[" << left1 << ", " << right1 << "]<-";
                        std::cout << "[" << left2 << ", " << right2 << "] ???????";
                        std::cout << "[" << coeff0.first << ", " << coeff2.first << "]";
                        std::cout << std::endl;
                    }

                    if(!(right1 - left1 + 1 <= skipflatmax && (acoeff1 <= flatthreshold0))){
                        /* dont merge with flat less than @skipflatmax elements */
                        break;
                        /* TODO try to lookahead to merge with them */
                    }
                    if(acoeff2 <= flatthreshold0)
                        break;
                    if(bm::sign(coeff2.first) != bm::sign(coeff0.first)){
                        break;
                    }
                    if(is_debug){
                        std::cout << "merging [" << left0 << ", " << right0 << "] <-";
                        std::cout << "[" << left1 << ", " << right1 << "]<-";
                        std::cout << "[" << left2 << ", " << right2 << "]";
                        std::cout << "[" << coeff0.first << ", " << coeff2.first << "]";
                        std::cout << std::endl;
                    }
                    i += 2;

                    std::get<1>(trend0) = right0 = right2;
                    auto newcoeff = get_linear_regr(std::make_pair(left0, right2), points);
                    std::get<2>(trend0) = distance0 = newcoeff.second;
                    if(newcoeff.first.first <= flatthreshold0){
                        newcoeff.first.first = 0;
                    }
                    std::get<3>(trend0) = coeff0 = newcoeff.first;
                    if(is_debug){
                        std::cout << "merging [" << left0 << ", " << right0 << "] <-";
                        std::cout << coeff0.first << std::endl;
                        std::cout << " +" << std::endl;
                    }
                }
            }
            if(is_debug)
            std::cout << "merging [" << left0 << ", " << right0 << "]" << std::endl;
            ranges.push_back(trend0);
        }

        return ranges;
    }

    template <bool is_debug = false>
    TrendsContainer merge_trends(const std::vector<std::pair<float, WT>> &points,
            const TrendsContainer &trends,
            const std::tuple<float, float, float, size_t> &thresholds){

        TrendsContainer newtrends = trends;
        size_t trends_size = 0;
        
        do{
            trends_size = newtrends.size();
            newtrends = merge_trends_basic<is_debug>(points, newtrends, thresholds);
            if(is_debug) std::cout << "loop1:" << std::endl << newtrends;
            //std::cout << std::endl << "Basic:" << std::endl << newtrends;
            newtrends = merge_trends_skipflats<is_debug>(points, newtrends, thresholds);
            if(is_debug) std::cout << "loop2:" << std::endl << newtrends;
            //newtrends = merge_trends_extremums(points, newtrends, thresholds);
            newtrends = merge_trends_basic<is_debug>(points, newtrends, thresholds);
            if(is_debug) std::cout << "loop3:" << std::endl << newtrends;
        }while(trends_size != newtrends.size());

        return newtrends;
    }

    TrendsContainer get_trends_extremums(const std::vector<std::pair<float, WT>> &points,
            const TrendsContainer &trends){
        TrendsContainer ranges;

        std::tuple<float, float, float, size_t> thresholds_ultimate{
            std::numeric_limits<float>::epsilon(), std::numeric_limits<float>::epsilon(),
            //0.015, 0.015,
            std::numeric_limits<float>::max(), std::numeric_limits<size_t>::max()
        };
        //remove flats :)
        ranges = merge_trends<false>(points, trends, thresholds_ultimate);
        return ranges;
    }

    /*
    algos: lin approx, flat 
        trend detection:
            1) detect flat trends: with flatness criteria
            2) between flat trends run lin approx algo
    */
    template<bool is_debug = false>
    TrendsContainer get_trends(
            const std::vector<std::pair<float, WT>> &points){

        DistanceResults thresholds = std::make_tuple(0.05, 1.0, 1.0, 1.0, 1.0, 1.0);

        TrendsContainer ranges = get_raw_trends<is_debug>(points, thresholds);
        if(is_debug){
        	std::cout << ranges;
        	print_best(points);
        }

        constexpr float flatthreshold0 = 0.015;
        constexpr float flatthreshold1 = 0.015;
        constexpr float similarthreshold = 0.50;
        constexpr size_t skipflatmax = 4;
        std::tuple<float, float, float, size_t> thresholds_basic{
            flatthreshold0, flatthreshold1, similarthreshold, skipflatmax
        };
        ranges = merge_trends<is_debug>(points, ranges, thresholds_basic);
        if(is_debug){
	        std::cout << "Merged:" << std::endl << ranges;
	    }
        //auto extremum_trends = get_trends_extremums(points, ranges);
        //std::cout << "Extremums:" << std::endl << extremum_trends;
        /* TODO merge trends smart: similar coeffs */

        return ranges;
    }

    template<bool is_debug = false>
    TrendsContainer detect(MetaSequence &seq, size_t index, size_t offset){
        std::vector<std::pair<float, WT>> flatpoints;
        //TODO seq = flatpoints
        //have X(X = window * 2 -1) points with values
        return get_trends<is_debug>(flatpoints);
    }

    template<bool is_debug = false>
    TrendsContainer detect(MetaSequence &seq, bt::ptime at_start, bt::ptime at_end){
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
                return detect<is_debug>(seq, index, offset - 1);
        }

        return detect<is_debug>(seq, index, index);
    }

    template<bool is_debug = false>
    TrendsContainer detect(MetaSequence &seq, bt::ptime at_end, bt::time_duration duration){
        bt::ptime at_start = at_end - duration;
        return detect<is_debug>(seq, at_start, at_end);
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