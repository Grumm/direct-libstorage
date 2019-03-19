#include "TradingAlgo.hpp"
#include "TechAnalysis/TechAnalysis.hpp"

/*
1) level hasn't been broken - with a big volume instant reversal of local trend.
    false positives - possible level retest with a break.
*/
void do_algo2(const std::vector<TicksSequence> &seqs){
    for(auto seq: seqs){
        ta::TrendDetector td(seq);

        bt::ptime t1(bg::date(2019,bg::Mar,12), bt::time_duration(19,30,00));
        bt::time_duration td1(12,00,00);
        td.detect(t1, td1, 100, 90, 300);
        //td.detect(240000, 20, 4, 2);
    }
}