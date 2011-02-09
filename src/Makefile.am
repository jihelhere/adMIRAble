AM_CXXFLAGS=

bin_PROGRAMS = ranker-learn-zcat ranker-predict count count_by_sentence drop_common_features transform_logprob filter filter_and_map test_utils split merge-models ranker-learn-iteration

ranker_learn_iteration_SOURCES = ranker-learn-iteration.cc utils.c
ranker_learn_iteration_CXXFLAGS = -O2 -g -Wall -Wextra -std=c++0x -ffast-math -pedantic
ranker_learn_iteration_LDFLAGS = -lm

ranker_learn_zcat_SOURCES = Example.hh ExampleMaker.hh MiraOperator.hh ranker-learn-zcat.cc utils.c utils.h
ranker_learn_zcat_CPPFLAGS= -DNDEBUG
ranker_learn_zcat_CXXFLAGS= -O6 -Wall -Wextra -std=c++0x -ffast-math -pedantic -pthread
ranker_learn_zcat_LDFLAGS= -lm -lpthread

ranker_predict_SOURCES = ranker-predict.cc
ranker_predict_CXXFLAGS = -O2 -g -Wall -Wextra -std=c++0x -ffast-math -pedantic
ranker_predict_LDFLAGS = -lm -lz


count_SOURCES = count.c hashtable.h hashtable.c
count_CFLAGS = -O2 -g -Wall -Wextra -ffast-math -pedantic
count_LDFLAGS = 

count_by_sentence_SOURCES = count_by_sentence.cc
count_by_sentence_CPPFLAGS= -DNDEBUG
count_by_sentence_CXXFLAGS = -O6 -Wall -Wextra -std=c++0x -ffast-math -pedantic
count_by_sentence_LDFLAGS = 

drop_common_features_SOURCES = drop_common_features.cc utils.c utils.h
drop_common_features_CXXFLAGS = -O2 -g -Wall -Wextra -ffast-math -pedantic -std=c++0x
drop_common_features_LDFLAGS = 

transform_logprob_SOURCES = transform_logprob.c
transform_logprob_CFLAGS = -O2 -g -Wall -Wextra -ffast-math -pedantic
transform_logprob_LDFLAGS= -lm

filter_SOURCES = filter.c hashtable.h hashtable.c
filter_CFLAGS = -O2 -g -Wall -Wextra -ffast-math -pedantic


filter_and_map_SOURCES = filter_and_map.c hashtable.h hashtable.c
filter_and_map_CFLAGS = -O2 -g -Wall -Wextra -ffast-math -pedantic

test_utils_SOURCES = test_utils.c utils.c
test_utils_CFLAGS = -Wall -Wextra -g 

split_SOURCES = split.cc utils.c
split_CXXFLAGS = -O2 -Wall -Wextra -g 

merge_models_SOURCES = merge-models.cc utils.c
merge_models_CXXFLAGS = -O2 -g -Wall -Wextra -std=c++0x
