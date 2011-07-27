#ifndef _EXAMPLEMAKER_HH_
#define _EXAMPLEMAKER_HH_

#include <vector>
#include <unordered_map>
#include <thread>

#include "example.hh"

namespace ranker {
struct ExampleMaker
{
    std::thread my_thread;

    std::vector<char*>& lines;
    std::vector<double>& weights;
    std::vector<example*> examples;

    int from;
    int to;

    ExampleMaker(std::vector<char*> &l, std::vector<double>& w)
        : lines(l), weights(w), examples() {};

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
            examples.push_back(new example(lines[i], weights));
        }
    }

    void start(int from, int to)
    {
        this->from = from;
        this->to = to > (int) lines.size() ? lines.size() : to;
        my_thread = std::thread(&ExampleMaker::create_example, this);
        //create_example();
    }

};
};



#endif
