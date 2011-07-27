#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "Example.hh"

namespace ranker {
    struct Predictor {
        int num_threads;
        std::unordered_map<std::string, int> mapping;
        std::unordered_map<int, double> model;

        Predictor(int num_threads, std::string modelname) {
            this->num_threads = num_threads;
            load_model(modelname);
        }

        Predictor(int num_threads, std::string modelname, std::string mappingname) {
            this->num_threads = num_threads;
            load_model(modelname);
            load_mapping(mappingname);
        }

        void load_model(std::string filename) {
            FILE* fp = fopen(filename.c_str(), "r");
            if(!fp) {
                fprintf(stderr, "ERROR: cannot load model from \"%s\"\n", filename.c_str());
                return;
            }

            size_t buffer_size = 0;
            char* buffer = NULL;
            int length = 0;

            while(0 < (length = getline(&buffer, &buffer_size, fp))) {
                buffer[length - 1] = '\0'; // chop
                char* weight1 = strchr(buffer, ' ');
                *weight1 = '\0';
                char* weight2 = strchr(weight1 + 1, ' ');
                if(weight2 != NULL) {
                    *weight2 = '\0';
                    model[strtol(buffer, NULL, 10)] = strtod(weight2 + 1, NULL);
                } else {
                    model[strtol(buffer, NULL, 10)] = strtod(weight1 + 1, NULL);
                }
            }
            fclose(fp);
        }
        void load_mapping(std::string filename) {
            FILE* fp = fopen(filename.c_str(), "r");
            if(!fp) {
                fprintf(stderr, "ERROR: cannot load mapping from \"%s\"\n", filename.c_str());
                return;
            }

            size_t buffer_size = 0;
            char* buffer = NULL;
            int length = 0;

            int next_id = 1;
            while(0 < (length = getline(&buffer, &buffer_size, fp))) {
                buffer[length - 1] = '\0'; // chop
                char* end = strchr(buffer, ' ');
                *end = '\0';
                auto found = model.find(next_id);
                if(found != model.end()) {
                    mapping[std::string(buffer)] = next_id;
                }
                next_id++;
            }
            fclose(fp);
        }

        int map(std::string feature) {
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
                score += model[i->id] * i->value;
            }
            x.score = score;
            return score;
        }
    };
}
