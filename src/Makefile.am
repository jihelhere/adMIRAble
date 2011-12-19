#AM_CPPFLAGS= -DNDEBUG
#AM_CXXFLAGS= -O6 

bin_PROGRAMS = ranker-learn ranker-predict count count_by_instance drop_common_features filter_and_map split merge-models ranker-learn-iteration ranker_main ranker_parallel

ranker_parallel_SOURCES = ranker_parallel.cc Predictor.hh utils.h utils.c
ranker_parallel_LDFLAGS = -pthread
if WANT_BOOST_THREAD
ranker_parallel_LDFLAGS+= $(BOOST_LDFLAGS) $(BOOST_SYSTEM_LIB) $(BOOST_THREAD_LIB)
endif

ranker_main_SOURCES = ranker_main.cc Predictor.hh utils.h utils.c

ranker_learn_iteration_SOURCES = ranker-learn-iteration.cc utils.c utils.h
ranker_learn_iteration_LDFLAGS = -lm

ranker_learn_SOURCES = example.hh exampleMaker.hh MiraOperator.hh ranker-learn.cc utils.c utils.h
ranker_learn_LDFLAGS= -lm -pthread
if WANT_BOOST_THREAD
ranker_learn_LDFLAGS+= $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB)
endif

ranker_predict_SOURCES = ranker-predict.cc utils.c utils.h
ranker_predict_LDFLAGS = -lm -lz

count_SOURCES = count.cc utils.c utils.h

count_by_instance_SOURCES = count_by_instance.cc utils.c utils.h

drop_common_features_SOURCES = drop_common_features.cc utils.c utils.h

filter_and_map_SOURCES = filter_and_map.cc utils.h utils.c

split_SOURCES = split.cc utils.c utils.h

merge_models_SOURCES = merge-models.cc utils.c
