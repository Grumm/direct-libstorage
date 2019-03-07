#!/bin/bash

cd obuild
cmake ../ -DCMAKE_BUILD_TYPE=$1
make
cp oa.exe ../
