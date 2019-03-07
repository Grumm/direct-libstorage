#!/bin/bash

root=`pwd`
#outdir=$root/raw-data/
outdir=/cygdrive/g/trading/data/brent-ice-raw/

outfiles=$outdir/*

./oa --input-raw-file $outfiles
