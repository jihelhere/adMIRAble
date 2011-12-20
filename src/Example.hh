#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <cassert>

namespace ranker {

struct Feature {
    unsigned id;
    double value;
    Feature(int _id, double _value) : id(_id), value(_value) {};
    inline bool operator<(const Feature &peer) const {
        return value < peer.value;
    }
};

struct Example {
  double loss;
  double score;
  std::vector<Feature> features;

  Example() : loss(0.0), score(0.0), features() {
    //    features.reserve(5000000);
  }


  Example(char* input) : loss(0.0), score(0.0), features() {
    //    features.reserve(5000000);
    load(input);
  }

  // load an example from a line 'loss feature_id:value .... feature_id:value' # comment
  // no error checking
  void load(char* line) {
    char * input = line;
    char *token = NULL;

    //TODO: uncomment this
    // char *comment = strchr(input, '#');
    // if(comment != NULL) *comment = '\0'; // skip comments

      token =  strsep(&input, " \t"); // read loss
      loss = strtod(token, NULL);
      for(;(token = strsep(&input, " \t\n")) && *token != '\0' ;) {
        char* value = strrchr(token, ':');

        *value = '\0';
        double value_as_double = strtod(value + 1, NULL);
        int feature_id = strtol(token, NULL, 10);
        features.emplace_back(feature_id, value_as_double);

      }
  }

  double compute_score(const std::vector<double>& weights) {
      score = 0;

      //      fprintf(stderr, "weights.size is: %ld\n", weights.size());

      for(auto i = features.begin(); i != features.end(); ++i) {
          if(i->id < weights.size()) score += i->value * weights[i->id];
      }
      return score;
  }

  // for sorting examples
  struct example_ptr_desc_score_order
  {
    inline bool operator()(const Example* __restrict__ i, const Example* __restrict__ j) const
    {return (i->score > j->score);}
  };

};

}
