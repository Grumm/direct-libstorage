#include "TradingAlgo.hpp"

/*
    all algos goal: calculate points of entry, with stoploss, where sl/tp statistically >>50%
*/

/*
    we need generic functions: 
        1) hypothesis: point gives profit?
            define profit: within X timeframes price statistically goes our
                direction for Y point with Z points drawback(sl)

                X Y Z - all variable, different for each PoE

        2) get to know period of time in which algo should be calculated

        3) could be more generic function - is_profit - what movement occurs after
            that point - as a function - could be simplified to a polynom.
*/

//first = tp, second = sl, third - timeframe when reached tp, 4th - is max sl hit
std::tuple<float, float, bt::time_duration, bool>
is_profit(const TicksSequence &seq, size_t index, bool is_long,
            bt::time_duration timeframe, float limit_max_sl){
    std::tuple<float, float, bt::time_duration, bool> tresult;
    //short = true
    //from seq[index] -> timeframe, check min price and remember max price before reaching min price
    //also could check for first price (at point) - L - lets say 40 points

    //limit_max_sl: if we have sl worse than max_sl, calculate tp up to this point
    auto t0 = seq.data[index];
    bt::ptime maxtime = t0.time + timeframe;
    bool is_limited_sl = false;
    bt::time_duration max_tp_timeframe = t0.time - t0.time;

    float max_tp = t0.price;
    float max_sl = t0.price;
    float max_sl_before_tp = t0.price;

    for(size_t i = index + 1; i < seq.data.size(); i++){
        Tick t = seq.data[i];
        if(t.time > maxtime)
            break;

        if(is_long){
            if(t.price > max_tp){
                max_tp = t.price;
                max_sl_before_tp = max_sl;
                max_tp_timeframe = t.time - t0.time;
            } else if (t.price < max_sl){
                max_sl = t.price;
            }
            if(t0.price - max_sl > limit_max_sl){
                is_limited_sl = true;
                break;
            }
        } else {
            if(t.price < max_tp){
                max_tp = t.price;
                max_sl_before_tp = max_sl;
                max_tp_timeframe = t.time - t0.time;
            } else if (t.price > max_sl){
                max_sl = t.price;
            }
            if(max_sl - t0.price > limit_max_sl){
                is_limited_sl = true;
                break;
            }
        }
    }

    if(is_long){
        max_sl_before_tp = t0.price - max_sl_before_tp;
        max_tp = max_tp - t0.price;
    } else {
        max_sl_before_tp = max_sl_before_tp - t0.price;
        max_tp = t0.price - max_tp;
    }

    return std::make_tuple(max_tp, max_sl_before_tp, max_tp_timeframe, is_limited_sl);
}

void test_is_profit0(const TicksSequence &seq, size_t index,
                            bool is_long, bt::time_duration td){
    auto [tp, sl, hit_tp_after, hit_max_sl] = is_profit(seq, index, is_long, td, 10);
    std::cout << "is_profit" << std::fixed << std::setprecision(2)
        << " tp " << tp
        << " sl " << sl
        << " hit_tp_after " << hit_tp_after
        << " hit_max_sl " << (hit_max_sl ? "true" : "false")
        << std::endl;
}

size_t find_seq_index_between_time(const TicksSequence &seq,
                                bt::ptime timestart){
    std::cout << "start" << timestart << std::endl;
    for(size_t i = 0; i < seq.data.size(); i++){
        auto tick = seq.data[i];
        if(tick.time > timestart){
            std::cout << "found " << tick.time << std::endl;
            return i;
        }
    }

    return 0;
}

void test_brent_may(const TicksSequence &seq){
    {
        bt::ptime t1(bg::date(2019,bg::Mar,6), bt::time_duration(11,30,00));
        size_t start_index = find_seq_index_between_time(seq, t1);
        {
            bt::time_duration td(00, 05, 0, 0);
            test_is_profit0(seq, start_index, false, td);
        }
        {
            bt::time_duration td(00, 30, 0, 0);
            test_is_profit0(seq, start_index, false, td);
        }
        {
            bt::time_duration td(01, 30, 0, 0);
            test_is_profit0(seq, start_index, false, td);
        }
        {
            bt::time_duration td(02, 30, 0, 0);
            test_is_profit0(seq, start_index, false, td);
        }
        {
            bt::time_duration td(03, 30, 0, 0);
            test_is_profit0(seq, start_index, false, td);
        }
    }
}

enum trend_t{
    trend_flat = 0,
    trend_long,
    trend_short,
};

struct PredictionResult{
    bt::time_duration duration;
    bt::time_duration tp_after;
    float tp;
    float sl;
    float tp_to_sl; // tp/sl
};

struct Prediction{
    size_t index;
    trend_t trend;

    std::vector<PredictionResult> results;

    Prediction(): index(0), trend(trend_flat){}

    Prediction(size_t index, trend_t trend): index(index), trend(trend){}
};

float calculate_price_weighted_window(const TicksSequence &seq, size_t index, size_t window){
    if(index == 0)
        return seq.data[index].price;

    uint64_t volume = 0;
    float weight = 0;
    for(ssize_t i = index - 1; i >= 0 && i >= (ssize_t)(index - 1 + 1 - window); i--){
        auto tick = seq.data[i];
        volume += tick.volume;
        weight += tick.price * tick.volume;
    }
    return weight / volume;
}

std::vector<Prediction> find_predictions(const TicksSequence &seq){
    constexpr uint64_t THRESH_VOL1 = 100;
    constexpr size_t WINDOW_SIZE1 = 30;
    constexpr float THRESH_TREND_PRICE1 = 0.05;

    std::vector<Prediction> result;
    for(size_t i = 0; i < seq.data.size(); i++){
        auto tick = seq.data[i];

        if(tick.volume > THRESH_VOL1){
            /* calculate trend before */
            float wp_before = calculate_price_weighted_window(seq, i, WINDOW_SIZE1);
            trend_t prev_trend;
            if(abs(wp_before - tick.price) < THRESH_TREND_PRICE1){
                prev_trend = trend_flat;
            } else if(wp_before < tick.price){
                prev_trend = trend_long;
            } else if(wp_before > tick.price){
                prev_trend = trend_short;
            }
            trend_t future_trend_predicted = trend_flat;
            switch(prev_trend){
                case trend_flat:
                    //prediction - price goes the way of i'th order
                    if(wp_before < tick.price){
                        future_trend_predicted = trend_long;
                    } else if(wp_before > tick.price) {
                        future_trend_predicted = trend_short;
                    }
                    break;
                case trend_long:
                    //prediction - trend reversal
                    future_trend_predicted = trend_short;
                    break;
                case trend_short:
                    future_trend_predicted = trend_long;
                    break;
            }
            if(future_trend_predicted != trend_flat){
                Prediction p(i, future_trend_predicted);
                result.push_back(p);
            }
        }
    }
    return result;
}

void check_predictions_simple(const TicksSequence &seq, std::vector<Prediction> &prs,
            const std::vector<bt::time_duration> &position_durations){

    for(auto &prediction: prs){
        for(auto duration: position_durations){
            auto [tp, sl, hit_tp_after, hit_max_sl] = is_profit(seq,
                prediction.index, prediction.trend == trend_long, duration, 10);

            PredictionResult pr;
            pr.tp = tp;
            pr.sl = sl;
            sl += 0.01;
            pr.tp_to_sl = tp / sl;
            pr.duration = duration;
            pr.tp_after = hit_tp_after;

            prediction.results.push_back(pr);
        }
    }
}

void print_predictions(const TicksSequence &seq, const std::vector<Prediction> &prs){
    std::cout << "Predicions:" << std::endl;
    for(auto prediction: prs){
        auto tick = seq.data[prediction.index];
        std::cout << "At [" << prediction.index << "]" << tick.time 
            << " price " << tick.price << " volume " << tick.volume
            << " " << (prediction.trend == trend_long ? "long" : "short") << std::endl;
        std::cout << "Results: " << std::endl;
        for(auto prres: prediction.results){
            std::cout << "     "
            << "TP/SL " << prres.tp_to_sl
            << " TP " << prres.tp
            << " SL " << prres.sl
            << " after " << prres.tp_after
            << " out of " << prres.duration
            << std::endl;
        }
    }
}

#if 0
struct PredictionsStats{
    struct _PredictionsStats{

    };
    const std::vector<bt::time_duration> &tds;
    std::map<bt::time_duration, _PredictionsStats> &stts_map;

    PredictionsStats(const std::vector<bt::time_duration> &tds): tds(tds){}
};

PredictionsStats calculate_pridictions_stats(const TicksSequence &seq,
        const std::vector<Prediction> &prs,
        const std::vector<bt::time_duration> &tds){
    PredictionsStats stats(tds);
}

print_stats()
#endif
/*
1) level hasn't been broken - with a big volume instant reversal of local trend.
    false positives - possible level retest with a break.
*/
void do_algo1(const std::vector<TicksSequence> &seqs){
    for(auto seq: seqs){
        print_sequence(seq, 1);

        test_brent_may(seq);
        auto prs = find_predictions(seq);

        std::vector<bt::time_duration> position_durations;
        position_durations.push_back(bt::time_duration(00, 02, 0, 0));
        position_durations.push_back(bt::time_duration(00, 05, 0, 0));
        position_durations.push_back(bt::time_duration(00, 15, 0, 0));
        position_durations.push_back(bt::time_duration(00, 30, 0, 0));
        position_durations.push_back(bt::time_duration(01, 30, 0, 0));
        position_durations.push_back(bt::time_duration(02, 30, 0, 0));
        check_predictions_simple(seq, prs, position_durations);

        print_predictions(seq, prs);
#if 0
        auto stats = calculate_pridictions_stats(seq, prs, position_durations);
        print_stats(stats);
#endif
    }
}


/*

lookout for any 250+ lots(brent) ticks, see previous trend up or down,
    see local trend for next few ticks, and if trend continues, false signal,
    if stops, SIGNAL found, thats the entry point

The problem is how we recognise local trend: https://imgur.com/a/ic6xYzi
hmmmmm
1) could guess width: lets say trading range width is 5points,
    check if trend direction is up(so we going down backwards),
    from pointzero check if each previous point < p0 with margin
    5p, new point is p0. if condition false, we either ended trend,
    or have an false negative, or need a wider range
2) could do smoothing of X points around one(could weight by volume), move point by point,
    at the end get just points sequence, very smooth, and guess its direction
    also question about when to stop, i.e. locality size


*/


/*
for later:
we also need a box that receives stream of ticks and at any point generates the signal,
sends it to signal processor

DATASOURCE -> SIGNALDETECTOR ----signal-----> PROCESSOR

 */