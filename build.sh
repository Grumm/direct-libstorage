#!/bin/bash

cd obuild
cmake ../ -DCMAKE_BUILD_TYPE=Debug
make
cp oa.exe ../
