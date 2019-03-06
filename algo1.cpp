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

	float max_sl = t0.price;
	float max_tp = t0.price;

	for(size_t i = index + 1; i < seq.data.size(); i++){
		Tick t = seq.data[i];
		if(t.time > maxtime)
			break;

		if(is_long){
			if(t.price > max_tp){
				max_tp = t.price;
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
				max_tp_timeframe = t.time - t0.time;
				max_tp = t.price;
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
		max_sl = t0.price - max_sl;
		max_tp = max_tp - t0.price;
	} else {
		max_sl = max_sl - t0.price;
		max_tp = t0.price - max_tp;
	}

	return std::make_tuple(max_tp, max_sl, max_tp_timeframe, is_limited_sl);
}

/*
1) level hasn't been broken - with a big volume instant reversal of local trend.
	false positives - possible level retest with a break.
*/
void do_algo1(const std::vector<TicksSequence> &seqs){
	for(auto seq: seqs){
		print_sequence(seq, 1);
		bt::time_duration td(1, 0, 0, 0);
		auto [tp, sl, hit_tp_after, hit_max_sl] = is_profit(seq, 0, true, td, 10);
		std::cout << "is_profit" << std::fixed << std::setprecision(2)
			<< " tp " << tp
			<< " sl " << sl
			<< " hit_tp_after " << hit_tp_after
			<< " hit_max_sl " << (hit_max_sl ? "true" : "false")
			<< std::endl;
	}
}