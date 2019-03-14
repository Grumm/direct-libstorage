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

class TradingStrategyInterface{
public:
    const TicksSequence &seq;
    size_t current_index;
    float balance;

    TradingStrategyInterface(const TicksSequence &seq, size_t index):
        seq(seq), current_index(index), balance(0.0){}

    virtual void nextTick(size_t newindex) = 0;
    virtual float finish() = 0; //close all positions and return balance
};

/* TODO move out actual algo out to template????? */
class TradingStrategyGeneric: public TradingStrategyInterface{
    static constexpr float comission = 0;//0.005;
    static constexpr float entry_error = 0;//0.02;

    float stop;
    float take;
    bt::time_duration timeout;
    bt::time_duration execution_delay;

    enum PositionState{
        state_nopos,
        state_open,
    };

    enum PositionDirection{
        pos_long,
        pos_short,
    };
    struct Position{
        enum PositionState state;
        enum PositionDirection dir;
        float open_price;
        float sl;
        float tp;
        Tick tick_opened;
    };

    enum PositionCloseReason{
        close_stop,
        close_take,
        close_reverse_signal,
        close_timeout,
    };

    struct PositionResult{
        Position pos;
        Tick tick_closed;
        enum PositionCloseReason reason;
    };

    Position position;
    std::vector<PositionResult> results;

    float calculate_profit(const PositionResult &pr){
        float result = 0;
        if(pr.pos.dir == pos_long){
            result = pr.tick_closed.price - pr.pos.open_price; //or pr.pos.tick_opened.price
        } else {
            result = pr.pos.open_price - pr.tick_closed.price;
        }

        return result - comission - entry_error;
    }

    void close_position(enum PositionCloseReason reason){
        if(position.state != state_open){
            return;
            throw std::invalid_argument("closing not open position");
        }

        auto tick = seq.data[current_index];
        
        PositionResult pr;
        pr.pos = position;
        pr.tick_closed = tick;
        pr.reason = reason;
        results.push_back(pr);

        float profit = calculate_profit(pr);
        if(0 && abs(profit) >= 0.10){
            std::cout << "Closed " << std::fixed << std::setprecision(2)
                << profit << " reason ";
                switch (reason){
                    case close_stop:
                        std::cout << "stop";
                        break;
                    case close_take:
                        std::cout << "stop";
                        break;
                    case close_reverse_signal:
                        std::cout << "reverse_signal";
                        break;
                    case close_timeout:
                        std::cout << "timeout";
                        break;
                }
            std::cout << std::endl;
        }

        balance += profit;

        position.state = state_nopos;
    }    

    enum trend_t{
        trend_flat = 0,
        trend_long,
        trend_short,
    };

    void open_position(trend_t future_trend_predicted, const Tick &tick){
        if(position.state == state_open) {
            //TODO check if oppisite direction
            auto newpos = future_trend_predicted == trend_long ? pos_long : pos_short;
            if(newpos == position.dir){
                //nvm
                return;
            } else {
                close_position(close_reverse_signal);
                //below we open position
            }
        }
        {
            position.state = state_open;
            position.tick_opened = tick;
            position.dir = (future_trend_predicted == trend_long ? pos_long : pos_short);
            position.open_price = seq.data[current_index].price; //ideal...........
            position.sl = position.open_price +
                    (position.dir == pos_long ? - 1 * stop : + 1 *stop);
            position.tp = position.open_price +
                    (position.dir == pos_long ? + 1 * take : - 1 *take);
            //position.opened = tick.time; in case we have a delay(like 300ms)
        }
    }

    float calculate_price_weighted_window(const TicksSequence &seq, size_t index, size_t window){
        if(index == 0)
            return seq.data[index].price;

        uint64_t volume = 0;
        float weight = 0;
        for(ssize_t i = index - 1; i >= 0 && i >= (ssize_t)(index - 1 + 1 - window); i--){
            auto tick = seq.data[i];
            volume += tick.volume;
            weight += tick.price;// * tick.volume;
        }
        return weight;// / volume;
    }

    void try_open_position(const Tick &tick){
        static constexpr uint64_t THRESH_VOL1 = 100;
        static constexpr size_t WINDOW_SIZE1 = 500;
        static constexpr float THRESH_TREND_PRICE1 = 0.10;
        if(tick.volume > THRESH_VOL1){
            /* calculate trend before */
            float wp_before = calculate_price_weighted_window(seq, current_index, WINDOW_SIZE1);
            trend_t prev_trend = trend_flat;
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
                //TODO open position
                open_position(future_trend_predicted, tick);
            }
        }
    }
public:
    TradingStrategyGeneric(const TicksSequence &seq, size_t index,
        float stop, float take,  bt::time_duration timeout,
        bt::time_duration execution_delay):
            TradingStrategyInterface(seq, index),
            stop(stop), take(take), timeout(timeout),
            execution_delay(execution_delay) {}

    void nextTick(size_t newindex){
        current_index = newindex;
        auto tick = seq.data[current_index];

        if(position.state == state_open){
            //check timeout
            bt::time_period tp(position.tick_opened.time, timeout);
            if(!tp.contains(tick.time)){
#if 0
                std::cout << "opened " << position.tick_opened.time
                << " now " << tick.time
                << " timeout " << timeout
                << " period " << tp
                <<std::endl;
#endif
                close_position(close_timeout);
            } else {
                //check sl/tp
                if(position.dir == pos_long){
                    if(tick.price < position.sl)
                        close_position(close_stop);
                    else if(tick.price > position.tp){
                        close_position(close_take);
                        //TODO or could do this - close half and s/l breakeven
                    }
                } else {
                    if(tick.price > position.sl)
                        close_position(close_stop);
                    else if(tick.price < position.tp){
                        close_position(close_take);
                    }
                }
            }
        }
        try_open_position(tick); //f1
        //f2, etc. TODO
    }

    float finish(){
        close_position(close_timeout);
        return balance;
    }
};

float emulate_trading_entry_points_by_strategy(TradingStrategyInterface *tsi){
    for(size_t i = 0; i < tsi->seq.data.size(); i++){
        tsi->nextTick(i);
    }
    return tsi->finish();
}

/*
1) level hasn't been broken - with a big volume instant reversal of local trend.
    false positives - possible level retest with a break.
*/
void do_algo1(const std::vector<TicksSequence> &seqs){
    float tresult = 0.0;
    for(auto seq: seqs){
        print_sequence(seq, 1);
#if 0
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
#endif
#if 0
        auto stats = calculate_pridictions_stats(seq, prs, position_durations);
        print_stats(stats);
#endif

        auto *ts1 = new TradingStrategyGeneric(seq, 0, 0.20, 0.40, 
            bt::time_duration(01, 15, 59, 0), bt::time_duration(00, 00, 2, 0));

        auto lres = emulate_trading_entry_points_by_strategy(ts1);
        std::cout << std::fixed << std::setprecision(2)
            << "result = " << lres << std::endl;
        tresult += lres;
    }
    std::cout << std::fixed << std::setprecision(2)
        << "total result = " << tresult << std::endl;
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


/*
TODO
1) implement trend detector - various time scale, for different volatility
2) volatility measure quantitavely for any period of time
3) volume trading levels for current contract (for market maker simulation)
4) level detection for volumes - today, past few days, current trading range
    with level strength
5) detect strong levels - quantitavely
6) just generic indicators - 200ma, 50ma, fibbonaci, volume reversals
7) detect trading range levels: |\/\/\/\/\/\/\/ pattern
8) other contracts at the same time - far end of curve, CL/BR, spread
8) options volumes TODO get contract options trading ticks/eod stats


combine function of all of the above, run simulation(taking into account
true positives, false positives) to detect which properties
actually have effect on result
function to search for parameters is under question, but could be simply brute force,
or genetic programming, or more complex AI method.........

*/

/*
https://imgur.com/a/XW1zsvL testing data for 08/03/2019
*/