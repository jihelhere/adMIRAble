#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "Example.hh"

namespace ranker {
    struct predictor {
        int num_threads;
        std::unordered_map<std::string, int> mapping;
        std::unordered_map<int, double> model;
        predictor(int num_threads, std::string modelname);
        predictor(int num_threads, std::string modelname, std::string mappingname);
        void load_model(std::string filename);
        void load_mapping(std::string filename);
        int map(std::string feature);
        int predict(std::vector<example>& nbest);
        double compute_score(example& i);
    };
}
