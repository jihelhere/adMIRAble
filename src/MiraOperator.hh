#ifndef _MIRA_OPERATOR_HH_
#define _MIRA_OPERATOR_HH_

#include <vector>
#include <unordered_map>
#include <algorithm>

#include "Example.hh"

namespace ranker {

    struct MiraOperator
    {
        int loop;
        int iteration;
        int num_examples;

        double clip;
        std::vector<double> &weights;
        std::vector<double> &avgWeights;

        int num;
        Example* oracle;

        MiraOperator(int loop_,  int iteration_, int num_examples_, double clip_,
                std::vector<double> &weights_, std::vector<double> &avgWeights_)
            :
                loop(loop_), iteration(iteration_), num_examples(num_examples_),
                clip(clip_),
                weights(weights_), avgWeights(avgWeights_),
                num(0), oracle(NULL)
        {};

        void update(Example* oracle_, int num_)
        {
            oracle = oracle_;
            num = num_;

        }

      inline
      bool incorrect_rank(const Example * example)
      {
	return example->score > oracle->score || (example->score == oracle->score && example->loss > oracle->loss);
      }


      void operator()(Example * example)
      {
        //        fprintf(stderr, "example: %g score: %g\n", example->score, example->loss);

	// skip the oracle -> useless update
	if(example == oracle) return;
	//fprintf(stderr, "mira: os: %g ol: %g es: %g el: %g\n", oracle->score, oracle->loss, example->score, example->loss);

	sort(oracle->features.begin(), oracle->features.end());
	sort(example->features.begin(), example->features.end());
	if(incorrect_rank(example)) {

	  //copy
	  auto i = oracle->features.begin(), j = example->features.begin();
	  double norm = 0;

	  while(i != oracle->features.end() && j != example->features.end()) {
	    if(i->id < j->id) {
	      norm += i->value * i->value;
	      ++i;
	    } else if(j->id < i->id) {
	      norm += j->value * j->value;
	      ++j;
	    } else {
	      double difference = i->value - j->value;
	      norm += difference * difference;
	      ++i;
	      ++j;
	    }
	  }
	  while(i != oracle->features.end())  { norm += i->value * i->value; ++i; }
	  while(j != example->features.end()) { norm += j->value * j->value; ++j; }


	  double alpha = 0.0;
	  double delta = example->loss - oracle->loss - (oracle->score - example->score);

	  delta /= norm;
	  alpha += delta;

	  if(alpha < 0) alpha = 0;
	  if(alpha > clip) alpha = clip;


	  double avgUpdate(double(loop * num_examples - (num_examples * ((iteration + 1) - 1) + (num )) + 1));

	  double avgalpha = alpha * avgUpdate;
	  //                fprintf(stderr, "norm: %g alpha: %g\n", norm, alpha);

	  //update weight vectors

          //fprintf(stderr, "%lu %lu\n", oracle->features.size(), example->features.size());


          sort(oracle->features.begin(), oracle->features.end());
          sort(example->features.begin(), example->features.end());
	  unsigned s = oracle->features.back().id > example->features.back().id ?
	    oracle->features.back().id : example->features.back().id ;


          //          fprintf(stderr, "before resizing to %d\n", s);
	  if(weights.size() <= s) {
	    weights.resize(s+1, 0);
	    avgWeights.resize(s+1, 0);
	  }
	  //		fprintf(stderr, "after resizing\n");

	  i = oracle->features.begin();
	  j = example->features.begin();

	  while(i != oracle->features.end() && j != example->features.end()) {
	    if(i->id < j->id) {
	      weights[i->id] += alpha * i->value;
	      avgWeights[i->id] += avgalpha * i->value;
	      ++i;
	    } else if(j->id < i->id) {
	      weights[j->id] -= alpha * j->value;
	      avgWeights[j->id] -= avgalpha * j->value;
	      ++j;

	    } else {
	      assert(i->id == j->id);
	      double difference = i->value - j->value;
	      weights[j->id] += alpha * difference;
	      avgWeights[j->id] += avgalpha * difference;
	      ++i;
	      ++j;
	    }
	  }
	  while(i != oracle->features.end())  {
	    weights[i->id] += alpha * i->value;
	    avgWeights[i->id] += avgalpha * i->value;
	    ++i;
	  }
	  while(j != example->features.end()) {
            //            fprintf(stderr, "size weights: %lu\n", weights.size());
	    weights[j->id] -= alpha * j->value;
            //            fprintf(stderr, "size avgWeights: %lu\n", avgWeights.size());
            //            fprintf(stderr, "j->id: %lu\n", j->id);
	    avgWeights[j->id] -= avgalpha * j->value;
	    ++j;
	  }
	}
      }
    };

}

#endif
