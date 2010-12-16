#!/bin/bash

if [ $# != 5 ]
then
    echo "USAGE: $0 <num-jobs> <iterations> <clip> <training-set> <model>" >&2
    exit 1
fi

numjobs=$1
iterations=$2
clip=$3
trainingset=$4
model=$5
tmp=ranker-learn.$$

function clean() {
    rm $tmp.*
}

trap INT clean

./split $trainingset $numjobs $tmp.training.

for iteration in `seq $iterations`
do
    for data in $tmp.training.*
    do
        echo ./ranker-learn-iteration $iteration $iterations $clip $data $model > $data.$iteration.model
    done | parallel -j $numjobs
    ./merge-models $tmp.training.*.$iteration.model > $model
done
