#pragma once


namespace ta{

using PriceType = float;
namespace bt = boost::posix_time;

#if 0
struct PriceType{
	uint32_t dec:24;
	uint32_t point:8;

	PriceType opAdd(const PriceType &p1, const PriceType &p2);
	PriceType opeartor+(const PriceType &other){
		return opAdd(this, other);
	}
};
#endif

struct PriceRange{
	PriceType start, end, min, max, wavg;
};

using SequencePointComposition = std::vector<AbstractSequencePoint *>;

//interface
class AbstractSequencePoint{
public:
    virtual const SequencePointComposition &getComposition() = 0;
    virtual PriceRange getPriceRange() = 0;
    virtual bt::time_period getTimePeriod() = 0;
    virtual PriceType getPrice() = 0;
    virtual uint64_t getVolume() = 0;
};

class SingleSequencePoint: public AbstractSequencePoint{
	PriceType price;
	uint64_t volume;
	bt::ptime time;
public:
	SingleSequencePoint(): price(0.0), volume(0), time(bt::ptime()){}
	SingleSequencePoint(PriceType price, uint64_t volume, bt::ptime time):
		price(price), volume(volume), time(time){}

    virtual const SequencePointComposition &getComposition() override
    	{ return SequencePointComposition{}; }
    virtual PriceRange getPriceRange() override { return PriceRange{price}; }
    virtual bt::time_period getTimePeriod() override {return time_period(time, time); }
    virtual PriceType getPrice() override { return price; }
    virtual uint64_t getVolume() override { return volume; }
};

class TimePeriodSequencePoint: public AbstractSequencePoint{
	PriceRange price_range;
	uint64_t volume;
	bt::time_period time_period;

	std::vector<SingleSequencePoint> points;
public:
	TimePeriodSequencePoint(): 
		price_range{}, volume(0), time_period(bt::ptime(), bt::ptime()){}
	TimePeriodSequencePoint(std::vector<SingleSequencePoint> points): points(points){
		//TODO calculate price_range, vol, t-p based on points
	}
	//different interface for this. Not clear who owns it
    virtual const SequencePointComposition &getComposition() override
    	{ return SequencePointComposition{}; }
    virtual PriceRange getPriceRange() override { return PriceRange{price}; }
    virtual bt::time_period getTimePeriod() override {return time_period; }
    virtual PriceType getPrice() override { return price; }
    virtual uint64_t getVolume() override { return volume; }
};


}