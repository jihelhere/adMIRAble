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
        std::vector<Example*> examples;

        int from;
        int to;

      ExampleMaker(std::vector<char*> &l, std::vector<double>& w) : my_thread(), lines(l), weights(w), examples() {};

        ~ExampleMaker() {
            for(auto example = examples.begin(); example != examples.end(); example++) {
                delete *example;
            }
        }

        void join() {
            my_thread.join();
        }


        void create_example()
        {
            for(int i = from; i < to; i++) {
                examples.push_back(new Example(lines[i]));
                examples.back()->compute_score(weights);
            }
        }

        void start(int from, int to)
        {
            this->from = from;
            this->to = to > (int) lines.size() ? lines.size() : to;

            my_thread = threadns::thread(&ExampleMaker::create_example, this);

            //create_example();
        }

    };
}



#endif
