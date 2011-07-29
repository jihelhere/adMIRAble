#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <string>

namespace ranker {

struct Feature {
    unsigned id;
    double value;
    Feature(int _id, double _value) : id(_id), value(_value) {};
    bool operator<(const Feature &peer) const {
        return value < peer.value;
    }
};

struct Example {
  double loss;
  double score;
  std::vector<Feature> features;
  
  Example() : loss(0.0), score(0.0), features() { }


  Example(const char* input) : loss(0.0), score(0.0), features() {
      load(input);
  }

  // load an example from a line 'loss feature_id:value .... feature_id:value' # comment
  // no error checking
  void load(const char* line) {
      features.clear();
      char *input = strdup(line);
      char *token = NULL; 
      char *comment = strchr(input, '#');
      if(comment != NULL) *comment = '\0'; // skip comments
      token =  strsep(&input, " \t"); // read loss
      loss = strtod(token, NULL);
      for(;(token = strsep(&input, " \t\n"));) {
          if(!strcmp(token,"")) continue;
          char* value = strrchr(token, ':');
          if(value != NULL) { // read feature_id:value
              *value = '\0';
              double value_as_double = strtod(value + 1, NULL);
              int feature_id = strtol(token, NULL, 10);
              features.push_back(ranker::Feature(feature_id, value_as_double));
          }
      }
      free(input);
  }

  double compute_score(std::vector<double> weights) {
      score = 0;
      for(auto i = features.begin(); i != features.end(); ++i) {
          score += i->value * weights[i->id];
      }
      return score;
  }

  // for sorting examples
  struct example_ptr_desc_score_order 
  {
      bool operator()(const Example* i, const Example* j) {return (i->score > j->score);}
  };

};

}
