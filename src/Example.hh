#pragma once

#define BERKELEY_HACK 1


#include <vector>
#include <string>

struct feature {
    unsigned id;
    double value;
    feature(int _id, double _value) : id(_id), value(_value) {};
    bool operator<(const feature &peer) const {
        return value < peer.value;
    }
};

struct example {
  double loss;
  double score;
#ifdef BERKELEY_HACK
  double prob;
#endif

  int label;
  std::vector<feature> features;
  
  example() : loss(0.0), score(0.0), 
#ifdef BERKELEY_HACK
	      prob(0.0), 
#endif
	      label(0), features() {};


  // create an example from a line 'label fts:val .... fts:val'
  // side-effects : update size of weights and avgweights
  // unknown features are *ignored*
  example(char*& buffer, std::vector<double>& weights)
    : loss(0.0), score(0.0), label(0), features()
  {
    // read a line and fill label/features
    
    char * inputstring = buffer;
    char *token = NULL; 
    if ((token =  strsep(&inputstring, " \t")) != NULL) {
      if(!strcmp(token, "1")) {
	this->label = 1;
      }
    } 
    
    for(;(token = strsep(&inputstring, " \t\n"));) {
      if(!strcmp(token,"")) continue;
      
      char* value = strrchr(token, ':');
      if(value != NULL) {
	*value = '\0';
	double value_as_double = strtod(value + 1, NULL);
	
	//assert(!isinf(value_as_double));
	//assert(!isnan(value_as_double));
	
	//nbe is the loss, not a feature
	if(!strcmp(token, "nbe")) {
	  this->loss = value_as_double;
    } else {

#ifdef BERKELEY_HACK
	  if(!strcmp(token, "1")) {
	    this->prob = value_as_double;
	  }
#endif
        int location = strtol(token, NULL, 10);
        features.push_back(feature(location, value_as_double));
        if(location < (int) weights.size()) this->score += value_as_double * weights[location];
        //fprintf(stderr, "%d:%g ", location, value_as_double);
    }
      }
    }
    //fprintf(stderr, "\n");
  }


  // for sorting examples
  struct example_ptr_desc_score_order 
  {
      bool operator()(const example* i, const example* j) {return (i->score > j->score);}
  };

#ifdef BERKELEY_HACK
  struct example_ptr_desc_prob_order 
  {
      bool operator()(const example* i, const example* j) {return (i->prob > j->prob);}
  };
#endif

};

