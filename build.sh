#!/bin/bash

g++ -O3 -flto -march=native algo1.cpp TradingAlgo.cpp main.cpp -o oa -lboost_program_options -lboost_date_time
#g++ -O0 -g -march=native algo1.cpp TradingAlgo.cpp main.cpp -o oa -lboost_program_options -lboost_date_time