#ifndef _EXAMPLE_H_
#define _EXAMPLE_H_

#include <vector>
#include <unordered_map>
#include <string>


struct example {
  double loss;
  double score;
  int label;
  std::unordered_map<int, double> features;
  
  example() : loss(0.0), score(0.0), label(0), features() {};


  // create an example from a line 'label fts:val .... fts:val'
  // side-effects : update size of weights and avgweights
  // unknown features are *ignored*
  example(char*& buffer, const std::unordered_map<std::string,int>& features,
	  std::vector<double>& weights)
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
	std::string token_as_string = token;
	double value_as_double = strtod(value + 1, NULL);
	
	assert(!isinf(value_as_double));
	assert(!isnan(value_as_double));
	
	//nbe is the loss, not a feature
	if(token_as_string == "nbe") {
	  this->loss = value_as_double;
	} 
	
	else {
	  if(features.find(token_as_string) == features.end()) {
	    // fprintf(stderr, "could not find %s\n", token_as_string.c_str());
	    continue;
	  }
	  
	  unsigned int location = features.at(token_as_string);
	  
	  this->features[location] = value_as_double;
	  this->score += value_as_double * weights[location];
	}
      }
    }
  }
  
  
  // for sorting examples
  struct example_ptr_desc_order 
  {
    bool operator()(const example* i, const example* j) {return (i->score > j->score);}
  };
};

#endif
