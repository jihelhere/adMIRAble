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
tmp=ranker-learn.6414
dir=`dirname $0`

function clean() {
    rm $tmp.*
    exit 1
}

trap clean SIGINT SIGTERM

examples=$($dir/split $trainingset $numjobs $tmp.training|head -1)

# reset model
echo -n > $model

for iteration in `seq $iterations`
do
    for data in $tmp.training.*.gz
    do
        echo "$dir/ranker-learn-iteration -e "$examples" -i $(expr $iteration - 1) -n $iterations -c $clip -t $data -m $model > $data.$iteration.model"
    done | tee -a /dev/stderr | parallel -j $numjobs
    $dir/merge-models $tmp.training.*.$iteration.model > $model
done

clean
