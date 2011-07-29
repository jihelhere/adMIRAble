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
   1. a c++0x-aware compiler, for example gcc > 4.0
   2. pigz, a parallel implementation of gzip

Usage
-----

Training:

   ranker-learn-zcat --train <training-file> --dev <dev-file> --test <test-file> [options]

Predictions:

   zcat examples | ranker_main <model> [num-candidates]

Data format
-----------

gzipped version of text file with lines like: 'loss feature_id:value ...
feature_id:value' Each instance must be separated by a blank line. For
example:

   0 1:43 2:34 5:21
   0.3 1:2 3:0.32

   1 1:-12 2:1.4
   0.01 3:1.7
   

Remark
------

   In the new file format, the loss is used in place of the label and there is
   no 'nbe' feature anymore.
