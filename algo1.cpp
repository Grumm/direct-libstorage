#include "TradingAlgo.hpp"

void do_algo1(const std::vector<TicksSequence> &seqs){
	for(auto seq: seqs){
		print_sequence(seq);
	}
}