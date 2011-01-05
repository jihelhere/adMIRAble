#ifndef _EXAMPLEMAKER_HH_
#define _EXAMPLEMAKER_HH_

#include <vector>
#include <unordered_map>
#include <thread>

#include "Example.hh"

struct example_maker
{
  std::thread my_thread;

  char* buffer;
  const std::unordered_map<std::string,int>& features;
  std::vector<double>& weights;

  example * my_example;


  example_maker(char* b, const std::unordered_map<std::string,int>& f, std::vector<double>& w)
    : buffer(b), features(f), weights(w), my_example(NULL) {};

  ~example_maker() { free(buffer);}

  void join() {my_thread.join();}


  void create_example()
  {
    char * b = buffer;
    my_example = new example(b, features, weights);
  }

  void start()
  {
    my_thread = std::thread(&example_maker::create_example, this);
  }
  
};



#endif
