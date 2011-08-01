#AM_CXXFLAGS=

bin_PROGRAMS = ranker-learn ranker-predict count count_by_instance drop_common_features filter_and_map split merge-models ranker-learn-iteration ranker_main ranker_parallel

ranker_parallel_SOURCES = ranker_parallel.cc Predictor.hh
ranker_parallel_LDFLAGS = -pthread

ranker_main_SOURCES = ranker_main.cc Predictor.hh

ranker_learn_iteration_SOURCES = ranker-learn-iteration.cc utils.c
ranker_learn_iteration_LDFLAGS = -lm

ranker_learn_SOURCES = example.hh exampleMaker.hh MiraOperator.hh ranker-learn.cc
ranker_learn_LDFLAGS= -lm -pthread

ranker_predict_SOURCES = ranker-predict.cc 
ranker_predict_LDFLAGS = -lm -lz

count_SOURCES = count.cc

count_by_instance_SOURCES = count_by_instance.cc

drop_common_features_SOURCES = drop_common_features.cc utils.c utils.h

filter_and_map_SOURCES = filter_and_map.cc

split_SOURCES = split.cc utils.c

merge_models_SOURCES = merge-models.cc utils.c
