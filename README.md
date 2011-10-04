adMIRAble : an admirable implementation of MIRA for reranking
=============================================================

Intro
-----

Given lists of examples with an associated loss, adMIRAble finds a linear model
that ranks the lowest-loss example first.  It is implemented as an averaged
perceptron with MIRA updates (PA-II algorithm in Crammer & Singer 2003).

Crammer, K., Singer, Y. (2003): Ultraconservative Online Algorithms for
Multiclass Problems. In: Journal of Machine Learning Research 3, 951-991.

Dependencies
------------

1. a c++0x-aware compiler, for example gcc > 4.4
2. autotools
3. if your OS doesn't provide a c++0x thread implementation, you need the boost library (only tested with boost 1.47)

Compiling
---------

    ./bootstrap
    ./configure --enable-debug=false [--with-boost=(yes|path_to_boost)]
    make
    make install

Usage
-----

Training:

    ranker-learn --train <training-file> --dev <dev-file> --test <test-file> [options]

Predictions (examples must contain an ignored value for the loss):

    cat examples | ranker_main <model> [num-candidates]

Utilities:

1. count: count number of occurence of features
2. count_by_instance: count number of instances that contain a feature
3. drop_common_features: drop features that are in all candidates of an instance
4. filter_and_map: remove features that appear less than n times according to a count file and map them to ids
5. mlcomp_to_reranker.py: create compatible training data from mlcomp/libsvm file format

Experimental parallel training:

1. ranker-learn-parallel.sh: main script
2. split: split corpus in several data shards
3. ranker-learn-iteration: one iteration of training on a subset of examples
4. merge-models: merge models at the end of iteration


Tutorial
--------

See test/run_test.sh

Note that you can gzip your example files for saving disk space. Then pass "--filter zcat" to ranker-learn.

Data format
-----------

gzipped version of text file with lines like: 'loss feature_id:value ...
feature_id:value' Each instance must be separated by a blank line. For
example:

    0 1:43 2:34 5:21     \
    0.3 1:2 3:0.32        -- one instance (3 candidates)
    2 2:3 3:4 19:-0.63   /

    1 1:-12 2:1.4
    0.01 3:1.7


Remark
------

In the new file format, the loss is used in place of the label and there is
no 'nbe' feature anymore.
