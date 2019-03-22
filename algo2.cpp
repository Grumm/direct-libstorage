#include "TradingAlgo.hpp"
#include "TechAnalysis/TechAnalysis.hpp"

using namespace ta;
/*
1) level hasn't been broken - with a big volume instant reversal of local trend.
    false positives - possible level retest with a break.
*/
void do_algo2(const std::vector<TicksSequence> &seqs){
    for(auto seq: seqs){

        bt::ptime t1(bg::date(2019,bg::Mar,01), bt::time_duration(00,00,00));
        bt::ptime t2(bg::date(2019,bg::Mar,12), bt::time_duration(19,30,00));
        bt::time_duration td1(12,00,00);

        TrendDetector td(seq);
        auto trends = td.detect(t1, t2, 100, 100, 300);
        std::cout << "Trends:" << std::endl << trends;

        VolumeProfile vp(seq);
        auto vmap = vp.detect(t1, t2);
        std::cout << "VolumeProfile:" << std::endl << vmap;

        //td.detect(240000, 20, 4, 2);
    }
}