#include <cstdlib>
#include <iostream>
#include <vector>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

namespace po = boost::program_options;

#include "TradingAlgo.hpp"

int main(int argc, char * argv[]){
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("input-raw-file", po::value< std::vector<std::string> >()->multitoken(), "input raw file")
        ("input-file", po::value< std::vector<std::string> >()->multitoken(), "input file to serialize")
        ("output-file", po::value< std::vector<std::string> >()->multitoken(), "output file for serialization")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc,
         po::command_line_style::unix_style ^ po::command_line_style::allow_short), vm);
    po::notify(vm);

    if (vm.count("input-raw-file")) {
        std::cout << "Raw input files are: ";
        auto files = vm["input-raw-file"].as< std::vector<std::string> >();
        for(auto f: files){
            std::cout << f << " ";
        }
        std::cout << std::endl;

        run_algo(files);
        return 0;
    }
    if (vm.count("input-file") && vm.count("input-file") == vm.count("output-file")) {
        std::cout << "Input files are: ";
        auto infiles = vm["input-file"].as< std::vector<std::string> >();
        for(auto f: infiles){
            std::cout << f << " ";
        }
        std::cout << std::endl;

        std::cout << "Output files are: ";
        auto outfiles = vm["output-file"].as< std::vector<std::string> >();
        for(auto f: outfiles){
            std::cout << f << " ";
        }
        std::cout << std::endl;

        serialize(infiles, outfiles);
        return 0;
    }
    if (vm.count("help")) {
        std::cout << desc;
        return 0;
    }

    return -1;
}