#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <fstream>

#include <boost/algorithm/string.hpp>
#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp" //include all types plus i/o


namespace bt = boost::posix_time;
namespace bg = boost::gregorian;
namespace ba = boost::algorithm;

std::vector<std::string> parse_line(const std::string &line, const std::string &del);

class TickOptionsInterface{
public:
    enum tickcombining{
        tc_avg,
        tc_weighted_avg,
        tc_sum,
        tc_linear_avg,//linear coeff and cut in half
        tc_median,
    };
    Tick method(const std::vector<Tick> &v);

};

class TickCombination{
public:
    enum period{
        time_duration_rounded,
        time_duration_percise,
        ticks_iteration,
        volume_iteration,
    };

    enum combination{
        average,
        weighted_average,
        sum_combination,
        linear_average,
        median,
    };

    combination c;
    period p;

    TickCombination(combination c, period p): c(c), p(p){}

    std::vector<Tick> get_period(const std::vector<Tick> &v, size_t index){
        //from index in @v select sub-vector based on options
    }

    //Tick combine(const TicksSequence &ts, const TicksSequence::vector_range &range){
    Tick combine(const std::vector<Tick> &v){
        Tick tick;

        switch(c){
            case average:
                for(auto t: v){
                    tick.price += t.price;
                    tick.volume += t.volume;
                }
                tick.price /= v.size();
                break;
            case weighted_average:
                for(auto t: v){
                    tick.price += t.price * t.volume;
                    tick.volume += t.volume;
                }
                tick.price /= tick.volume;
                break;
            case sum_combination:
                throw std::runtime_error("not implemented combination");
                break;
            case linear_average:
                throw std::runtime_error("not implemented combination");
                break;
            case median:
                throw std::runtime_error("not implemented combination");
                break;
        }
        tick.time_period = bt::time_period(v.begin()->time_period.begin(),
            v.rbegin()->time_period.last());

        return tick;
    }
};

struct Tick{
    bt::time_period time_period;
    float price;
    uint64_t volume;

    Tick(): time_period(bt::ptime(), bt::ptime()), price(0.0), volume(0){}

    Tick(bt::ptime time, float price, uint64_t volume):
        time_period(time, time), price(price), volume(volume){}

    Tick(bt::ptime time1, bt::ptime time2, float price, uint64_t volume):
        time_period(time1, time2), price(price), volume(volume){}

    Tick(const std::vector<Tick> &v, const TickCombination &tc){
        *this = tc.combine(v);
    }
};

struct Candle{
    bt::ptime time_start; //any period of time
    bt::ptime time_end;

    float price_low;
    float price_high;

    float price_start;
    float price_end;

    uint64_t volume;
    float value; //sum of all volume*price

    std::vector<Tick> &ticks;

    Candle &operator+(const Tick &t){
        //TODO
        return *this;
    }
};

class TicksSequence{
private:
    /*
    https://www.cmegroup.com/month-codes.html
    <TICKER>;<PER>;<DATE>;<TIME>;<LAST>;<VOL>
    NYMEX.CL H19;0;15/01/19;00:00:07;51.540000000;1
    */
    //1)tokenize
    //2) convert datetime
    std::tuple<bt::ptime, float, uint64_t> parse_string(const std::string &line){
        bt::ptime time;
        float price = 0.0;
        uint64_t volume = 0;

        auto tokens = parse_line(line, ";");
        if(tokens.size() != 6)
            return std::make_tuple(time, price, volume);

        auto timetokens = parse_line(tokens[2], "/");
        if(timetokens.size() != 3)
            return std::make_tuple(time, price, volume);

        auto psxdate = "20" + timetokens[2] + "-" + timetokens[0] + "-" + timetokens[1];
        auto tstr = psxdate + " " + tokens[3];
        
        time = bt::time_from_string(tstr);
        price = std::stof(tokens[4]);
        volume = std::stoll(tokens[5]);

        return std::make_tuple(time, price, volume);
    }

public:
    using vector_range = std::pair<size_t, size_t>;

    std::string ticker;
    std::vector<Tick> data;

    TicksSequence(const std::string &ticker): ticker(ticker){}
    void push(const Tick &tick){
        data.push_back(tick);
    }
    void push(const std::string &line){
        auto [time, price, vol] = parse_string(line);
        Tick tick(time, price, vol);
        data.push_back(tick);
    }
};

std::ostream & operator<< (std::ostream &out, const Tick &tick);
void print_sequence(const TicksSequence &seq, int64_t limit = std::numeric_limits<int64_t>::max());

void serialize(const std::vector<std::string> &infiles,
                    const std::vector<std::string> &outfiles);
void run_algo(const std::vector<std::string> &files);
void do_algo1(const std::vector<TicksSequence> &seqs);
void do_algo2(const std::vector<TicksSequence> &seqs);

#if 0

std::time_t pt_to_time_t(const bt::ptime& pt)
{
    bt::ptime timet_start(boost::gregorian::date(1970,1,1));
    bt::time_duration diff = pt - timet_start;
    return diff.ticks()/bt::time_duration::rep_type::ticks_per_second;

}
void seconds_from_epoch(const std::string& s)
{
    bt::ptime pt;
    for(size_t i=0; i<formats_n; ++i)
    {
        std::istringstream is(s);
        is.imbue(formats[i]);
        is >> pt;
        if(pt != bt::ptime()) break;
    }
    std::cout << " ptime is " << pt << '\n';
    std::cout << " seconds from epoch are " << pt_to_time_t(pt) << '\n';
}

#endif