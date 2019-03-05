#!/bin/bash

root=`pwd`
outdir=$root/raw-data/

outfiles=$outdir/*

./oa --input-raw-file $outfiles
