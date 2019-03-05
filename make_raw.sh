#!/bin/bash

root=`pwd`
indir=$root/data/
outdir=$root/raw-data/


#infiles=`ls $indir --quoting-style={escape,shell,c}`
infiles=$indir/*
for f in $infiles
do
    fbname=$(basename "$f" .txt)
    echo $fbname
    ./oa --input-file "$indir/$fbname.txt" --output-file "$outdir/$fbname.rawr"
done

outfiles=$outdir/*
for f in $outfiles
do
    fbname=$(basename "$f")
    echo $fbname
    ./oa --input-raw-file "$outdir/$fbname"
done