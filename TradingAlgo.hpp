#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <fstream>

#include <boost/algorithm/string.hpp>
#include <boost/date_time.hpp>

namespace bt = boost::posix_time;
namespace bg = boost::gregorian;
namespace ba = boost::algorithm;

std::vector<std::string> parse_line(const std::string &line, const std::string &del);

struct Tick{
	bt::ptime time;
	float price;
	uint64_t volume;

	/*
	<TICKER>;<PER>;<DATE>;<TIME>;<LAST>;<VOL>
	NYMEX.CL H19;0;15/01/19;00:00:07;51.540000000;1
	*/
	//1)tokenize
	//2) convert datetime
	Tick(): price(0.0), volume(0){}
	Tick(const std::string &line): price(0.0), volume(0){
		auto tokens = parse_line(line, ";");
		if(tokens.size() != 6)
			return;

		auto timetokens = parse_line(tokens[2], "/");
		if(timetokens.size() != 3)
			return;
		auto psxdate = "20" + timetokens[2] + "-" + timetokens[1] + "-" + timetokens[0];
		auto tstr = psxdate + " " + tokens[3];
		time = bt::time_from_string(tstr);
		price = std::stof(tokens[4]);
		volume = std::stoll(tokens[5]);
	}
	//Tick(const Tick &t): time(t.time), price(t.price), volume(t.volume){}
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
public:
	std::string ticker;
	std::vector<Tick> data;

	TicksSequence(const std::string &ticker): ticker(ticker){}
	void push(const Tick &tick){
	}
	void push(const std::string &line){
		Tick tick(line);
		data.push_back(tick);
	}
};

std::ostream & operator<< (std::ostream &out, const Tick &tick);
void print_sequence(const TicksSequence &seq, int64_t limit = std::numeric_limits<int64_t>::max());

void serialize(const std::vector<std::string> &infiles,
					const std::vector<std::string> &outfiles);
void run_algo(const std::vector<std::string> &files);
void do_algo1(const std::vector<TicksSequence> &seqs);

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