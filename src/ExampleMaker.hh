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

  typedef threadns::unique_lock<threadns::mutex> lock_type;

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

    void create_examples(threadns::mutex* mlines, int* processed_lines, int* finished, threadns::condition_variable* cond_process, threadns::mutex* mutex_examples)
    {
      while(1) {
	int index;
	char * string;

        lock_type lock(*mlines);

        while(((unsigned) *processed_lines == lines.size()) && !*finished) {
#ifdef USE_BOOST_THREAD
	  //cond_process->wait(lock); // no deadlock with boost implementation  ?
          cond_process->timed_wait(lock, boost::posix_time::milliseconds(10));
#else
	  // prevent deadlock with posix implementation
	 cond_process->wait_for(lock, std::chrono::duration<int,std::milli>(10));
#endif
        }

	if(*finished && ((unsigned) *processed_lines == lines.size())) { lock.unlock(); break;}

	index = (*processed_lines)++;
	string = lines[index];

        lock.unlock();

	Example * e = new Example(string);
	e->compute_score(weights);


        lock_type lock2(*mutex_examples);
        if (examples.size() < lines.size())
          examples.resize(lines.size(),NULL);
	examples[index] = e;
        lock2.unlock();

        delete lines[index];

      }
    }

    void start(threadns::mutex* mlines, int* processed_lines, int* finished, threadns::condition_variable* cond_process, threadns::mutex* mutex_examples)
    {
      my_thread = threadns::thread(&ExampleMaker::create_examples, this,
				   mlines, processed_lines, finished, cond_process,
                                   mutex_examples);
    }


  };
}



#endif
