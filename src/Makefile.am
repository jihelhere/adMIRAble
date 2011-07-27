#AM_CXXFLAGS=

bin_PROGRAMS = ranker-learn-zcat ranker-predict count count_by_sentence drop_common_features filter_and_map split merge-models ranker-learn-iteration ranker_main ranker_parallel

lib_LTLIBRARIES = libranker-0.1.la
libranker_0_1_la_SOURCES = predictor.cc
libranker_0_1_la_CXXFLAGS = $(CXXFLAGS)

ranker_parallel_SOURCES = ranker_parallel.cc predictor.cc
ranker_parallel_LDFLAGS = -lpthread

ranker_main_SOURCES = ranker_main.cc predictor.cc

ranker_learn_iteration_SOURCES = ranker-learn-iteration.cc utils.c
ranker_learn_iteration_LDFLAGS = -lm

ranker_learn_zcat_SOURCES = example.hh exampleMaker.hh MiraOperator.hh ranker-learn-zcat.cc utils.c utils.h
ranker_learn_zcat_LDFLAGS= -lm -lpthread

ranker_predict_SOURCES = ranker-predict.cc predictor.cc
ranker_predict_LDFLAGS = -lm -lz

count_SOURCES = count.cc

count_by_sentence_SOURCES = count_by_sentence.cc

drop_common_features_SOURCES = drop_common_features.cc utils.c utils.h

filter_and_map_SOURCES = filter_and_map.cc

split_SOURCES = split.cc utils.c

merge_models_SOURCES = merge-models.cc utils.c
