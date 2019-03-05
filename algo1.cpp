#include "TradingAlgo.hpp"

/*
1) level hasn't been broken - with a big volume instant reversal of local trend.
	false positives - possible level retest with a break.
*/

void do_algo1(const std::vector<TicksSequence> &seqs){
	for(auto seq: seqs){
		print_sequence(seq, 0);
	}
}