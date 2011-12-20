#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "Example.hh"

#include "utils.h"


namespace ranker {
    struct Predictor {
        int num_threads;
        std::unordered_map<std::string, int> mapping;
        std::vector<double> model;

        Predictor(int num_threads, const std::string& modelname) {
            this->num_threads = num_threads;
            load_model(modelname);
        }

        Predictor(int num_threads, const std::string& modelname, const std::string& mappingname) {
            this->num_threads = num_threads;
            load_model(modelname);
            load_mapping(mappingname);
        }

        void load_model(const std::string& filename) {
            FILE* fp = fopen(filename.c_str(), "r");
            if(!fp) {
                fprintf(stderr, "ERROR: cannot load model from \"%s\"\n", filename.c_str());
                return;
            }

            size_t buffer_size = 0;
            char* buffer = NULL;
            int length = 0;

            while(0 < (length = read_line(&buffer, &buffer_size, fp))) {
                buffer[length - 1] = '\0'; // chop
                char* weight1 = strchr(buffer, ' ');
                *weight1 = '\0';
                char* weight2 = strchr(weight1 + 1, ' ');
                int id = strtol(buffer, NULL, 10);
                if((int) model.size() <= id) model.resize(id + 1);
                if(weight2 != NULL) {
                    *weight2 = '\0';
                    model[id] = strtod(weight2 + 1, NULL);
                } else {
                    model[id] = strtod(weight1 + 1, NULL);
                }
            }
            fclose(fp);
        }
        void load_mapping(const std::string& filename) {
            FILE* fp = fopen(filename.c_str(), "r");
            if(!fp) {
                fprintf(stderr, "ERROR: cannot load mapping from \"%s\"\n", filename.c_str());
                return;
            }

            size_t buffer_size = 0;
            char* buffer = NULL;
            int length = 0;

            int next_id = 1;
            while(0 < (length = read_line(&buffer, &buffer_size, fp))) {
                buffer[length - 1] = '\0'; // chop
                char* end = strchr(buffer, ' ');
                *end = '\0';
                if(next_id < (int) model.size()) mapping[std::string(buffer)] = next_id;
                next_id++;
            }
            fclose(fp);
        }

        int map(const std::string& feature) {
            auto found = mapping.find(feature);
            if(found != mapping.end()) return found->second;
            return 0;
        }

        int predict(std::vector<Example>& nbest) {
            int argmax = -1;
            double max = 0;
            for(size_t i = 0; i < nbest.size(); i++) {
                compute_score(nbest[i]);
                if(argmax == -1 || nbest[i].score > max) {
                    max = nbest[i].score;
                    argmax = i;
                }
            }
            return argmax;
        }

        double compute_score(Example& x) {
            double score = 0;
            for(auto i = x.features.begin(); i != x.features.end(); i++) {
                if(i->id < model.size()) score += model[i->id] * i->value;
            }
            x.score = score;
            return score;
        }
    };
}
