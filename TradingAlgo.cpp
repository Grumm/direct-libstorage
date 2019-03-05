#include "TradingAlgo.hpp"

#include <boost/serialization/vector.hpp>


std::vector<TicksSequence> fill_tick_sequence(const std::vector<std::string> &files){
	std::vector<TicksSequence> tsqs;
	for(auto file: files){
		std::ifstream infile(file);

		std::string base_filename = file.substr(file.find_last_of("/\\") + 1);
		std::string::size_type const p(base_filename.find_last_of('.'));
		std::string file_without_extension = base_filename.substr(0, p);
		//TODO might be more advanced parse
		TicksSequence ts(file_without_extension);

		std::string line;
		getline(infile, line); //first line with headers
		for(; getline(infile, line);){
			ts.push(line);
		}
		tsqs.push_back(ts);
	}

	return tsqs;
}
std::vector<std::string> parse_line(const std::string &line, const std::string &del){
	std::vector<std::string> tokens;
	boost::algorithm::split(tokens, line, ba::is_any_of(del));
	return tokens;
}

std::ostream & operator<< (std::ostream &out, const Tick &tick)
{
    out << tick.time << " " << tick.price << " " << tick.volume;
    return out;
}

void print_sequence(const TicksSequence &seq){
	std::cout << seq.ticker << std::endl;
	for(auto tick: seq.data){
		std::cout << tick << std::endl;
		return;
	}
}

static void serialize(const std::vector<TicksSequence> &seqs,
				const std::vector<std::string> &outfiles){
	if(seqs.size() != outfiles.size())
		throw std::invalid_argument("in files != out files");
	for(size_t i = 0; i < seqs.size(); i++){
		auto seq = seqs[i];
		std::ofstream ofs(outfiles[i]);
		std::string header = std::to_string(seq.data.size()) + ";" + seq.ticker + "\n";
		ofs.write(header.c_str(), header.size());
		ofs.write((const char *)seq.data.data(), seq.data.size() * sizeof(decltype(seq.data)::value_type));
	}
}

static std::vector<TicksSequence> deserialize(const std::vector<std::string> &files){
	std::vector<TicksSequence> seqs;

	for(auto f: files){
		std::ifstream ifs(f);

		std::string line;
		getline(ifs, line); //first line with headers
		auto tokens = parse_line(line, ";");
		if(tokens.size() != 2)
			throw std::invalid_argument("invalid header in raw file");

		auto vectorsize = std::stoll(tokens[0]);
		if(vectorsize <= 0)
			throw std::invalid_argument("invalid array size in raw file");
		TicksSequence seq(tokens[1]);
		seq.data.resize(vectorsize);

		ifs.read((char *)seq.data.data(), vectorsize * sizeof(decltype(seq.data)::value_type));

		seqs.push_back(seq);
	}

	return seqs;
}

void serialize(const std::vector<std::string> &infiles,
					const std::vector<std::string> &outfiles){
	//for each file read and store info in memory
	try{
		auto seqs = fill_tick_sequence(infiles);
		serialize(seqs, outfiles);
	}catch(const std::exception& e) {
	    std::cout << e.what() << std::endl;
	}
}

void run_algo(const std::vector<std::string> &files){
	//for each file read and store info in memory
	try{
		auto seqs = deserialize(files);
		do_algo1(seqs);
	}catch(const std::exception& e) {
	    std::cout << e.what() << std::endl;
	}
}