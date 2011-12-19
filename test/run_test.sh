#!/bin/bash

url=http://www.csie.ntu.edu.tw/~cjlin/libsvmtools/datasets/multiclass
dataset=letter.scale
bin=../src

for i in tr t val
do
    [ -f $dataset.$i.gz ] || curl $url/$dataset.$i | python $bin/mlcomp_to_reranker.py 26 | gzip > $dataset.$i.gz
done

[ -f $dataset.tr.dict ] || gunzip -c $dataset.tr.gz | $bin/count_by_instance > $dataset.tr.dict

for i in tr t val
do
    [ -f $dataset.$i.mapped.gz ] || gunzip -c $dataset.$i.gz | $bin/filter_and_map $dataset.tr.dict 1 | gzip > $dataset.$i.mapped.gz
done

[ -f $dataset.model ] || $bin/ranker-learn -s $dataset.tr.mapped.gz -d $dataset.val.mapped.gz -t $dataset.t.mapped.gz -c 1 -i 10 -f "gunzip -c" -j 4 > $dataset.model
gunzip -c $dataset.val.mapped.gz | $bin/ranker_main $dataset.model > $dataset.val.out
gunzip -c $dataset.t.mapped.gz | $bin/ranker_main $dataset.model > $dataset.t.out
