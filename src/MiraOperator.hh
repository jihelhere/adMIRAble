#ifndef _MIRA_OPERATOR_HH_
#define _MIRA_OPERATOR_HH_

#include <vector>
#include <unordered_map>
#include <algorithm>

#include "Example.hh"


struct mira_operator
{
  double avgUpdate;

  double clip;
  std::vector<double> &weights;
  std::vector<double> &avgWeights;
  example* oracle;

  mira_operator(int loop, int num_examples, int iteration, int num, double clip_,
		std::vector<double> &weights_, std::vector<double> &avgWeights_, 
		example* oracle_)
    : 
    avgUpdate(double(loop * num_examples - (num_examples * ((iteration + 1) - 1) + (num + 1)) + 1)),
    clip(clip_),
    weights(weights_), avgWeights(avgWeights_), oracle(oracle_)
  {};

  inline
  bool incorrect_rank(const example * example) 
  {
    return example->score > oracle->score || (example->score == oracle->score && example->loss > oracle->loss);
  }


  void operator()(example * example)
  {
    //fprintf(stdout, "%g %g\n", example->score, example->loss);
    
    // skip the oracle -> useless update
    if(example == oracle) return;
    //fprintf(stderr, "mira: %g %g %g %g\n", oracle->score, oracle->loss, example->score, example->loss);
    if(avgWeights.size() < weights.size()) avgWeights.resize(weights.size(), 0);
    
    
    sort(example->features.begin(), example->features.end());
    if(incorrect_rank(example)) {
      double alpha = 0.0;
      double delta = example->loss - oracle->loss - (oracle->score - example->score);
    
      //copy
      auto i = oracle->features.begin(), j = example->features.begin();
      double norm = 0;
      
      while(i != oracle->features.end() && j != example->features.end()) {
          if(i->id < j->id) {
              norm += i->value * i->value;
              i++;
          } else if(j->id < i->id) {
              norm += j->value * j->value;
              j++;
          } else {
              double difference = i->value - j->value;
              norm += difference * difference;
              i++;
              j++;
          }
      }
      while(i != oracle->features.end()) { norm += i->value * i->value; i++; }
      while(j != example->features.end()) { norm += j->value * j->value; j++; }

      delta /= norm;
      alpha += delta;
      if(alpha < 0) alpha = 0;
      if(alpha > clip) alpha = clip;
      double avgalpha = alpha * avgUpdate;
      //fprintf(stderr, "norm: %g alpha: %g\n", norm, alpha);

      //update weight vectors
      i = oracle->features.begin();
      while(i != oracle->features.end()) {
          weights[i->id] += alpha * i->value;
          avgWeights[i->id] += avgalpha * i->value;
          i++;
      }
      j = example->features.begin();
      while(j != example->features.end()) {
          weights[j->id] -= alpha * j->value;
          avgWeights[j->id] -= avgalpha * j->value;
          j++;
      }
    }
  }
};

#endif
