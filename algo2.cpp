#include "TradingAlgo.hpp"
#include "TechAnalysis/TechAnalysis.hpp"

/*
1) level hasn't been broken - with a big volume instant reversal of local trend.
    false positives - possible level retest with a break.
*/
void do_algo2(const std::vector<TicksSequence> &seqs){
    for(auto seq: seqs){
        ta::TrendDetector td(seq);

        bt::ptime t1(bg::date(2019,bg::Mar,12), bt::time_duration(14,07,00));
        bt::time_duration td1(00,30,00);
        td.detect(t1, td1, 10, 9);
        //td.detect(240000, 20, 4, 2);
    }
}