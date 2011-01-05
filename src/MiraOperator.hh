#ifndef _MIRA_OPERATOR_HH_
#define _MIRA_OPERATOR_HH_

#include <vector>
#include <unordered_map>

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
    
    
    if(incorrect_rank(example)) {
      double alpha = 0.0;
      double delta = example->loss - oracle->loss - (oracle->score - example->score);
    
      //copy
      std::unordered_map<int, double> difference = oracle->features;
      double norm = 0;
      
      std::unordered_map<int, double>::iterator end = example->features.end();
      
      for(auto j = example->features.begin(); j != end; ++j) {
	double&ref = difference[j->first];
	ref -= j->second;
	norm += ref*ref;
      }

      delta /= norm;
      alpha += delta;
      if(alpha < 0) alpha = 0;
      if(alpha > clip) alpha = clip;
      double avgalpha = alpha * avgUpdate;

      //update weight vectors
      end = difference.end();
      for(auto j = difference.begin(); j != end; ++j) {
	weights[j->first] += alpha * j->second;
	avgWeights[j->first] += avgalpha * j->second;
      }
    }
  }
};

#endif
