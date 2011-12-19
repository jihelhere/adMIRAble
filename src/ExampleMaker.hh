#ifndef _EXAMPLEMAKER_HH_
#define _EXAMPLEMAKER_HH_

#include <vector>
#include <unordered_map>

#ifdef USE_BOOST_THREAD
#include <boost/thread.hpp>
namespace threadns = boost;
#else
#include <thread>
namespace threadns = std;
#endif

#include "Example.hh"

namespace ranker {
  struct ExampleMaker
  {

    threadns::thread my_thread;

    std::vector<char*>& lines;
    std::vector<double>& weights;
    std::vector<Example*>& examples;

    ExampleMaker(std::vector<char*> &l, std::vector<double>& w, std::vector<Example*>& e) 
      : my_thread(), lines(l), weights(w), examples(e) {};

    ~ExampleMaker() {}

    void join() {
      my_thread.join();
    }

    void create_examples(std::mutex* mlines, int* processed_lines, int* finished, std::condition_variable* cond_process)
    {
      while(1) {
	int index;
	char * string = NULL;

	std::unique_lock<std::mutex> lock(*mlines);
	
	while(((unsigned) *processed_lines >= lines.size()) && !*finished) {
	  cond_process->wait(lock);
	}
	
	if(*finished && ((unsigned) *processed_lines == lines.size())) break;

	index = (*processed_lines)++;
	examples.resize(lines.size(),NULL);
	string = lines[index];
   
	lock.unlock();
	  
	Example * e = new Example(string);
	e-> compute_score(weights);
	    
	lock.lock();
	examples[index] = e;
	delete lines[index];
	lock.unlock();
      
      }
    }

    void start(std::mutex* mlines, int* processed_lines, int* finished, std::condition_variable* cond_process)
    {
      my_thread = threadns::thread(&ExampleMaker::create_examples, this,
				   mlines, processed_lines, finished, cond_process);
    }


  };
}



#endif
