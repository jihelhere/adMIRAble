#pragma once

#ifdef USE_BOOST_THREAD
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
namespace threadns = boost;
#else
#include <mutex>
#include <condition_variable>
namespace threadns = std;
#endif

template <class T>
class ResultVector;

template <class T>
class Result {
    public:
    int id;
    ResultVector<T>* origin;
    Result(int _id, ResultVector<T>* _origin) : id(_id), origin(_origin) {}
    Result() {}
    void set_result(T value) {
        origin->set_result(id, value);
    }
};

template <class T>
class ResultVector {
    threadns::mutex guard;
    threadns::condition_variable is_ready;

    std::vector<T> results;
    size_t num_set;
    bool complete;
    public:
    ResultVector(): num_set(0), complete(false) {
    }
    int add_result() {
        threadns::unique_lock<boost::mutex> l(guard);
        results.push_back(T());
        return results.size() - 1;
    }
    void set_result(int id, T& value) {
        threadns::unique_lock<boost::mutex> l(guard);
        assert(!(complete && num_set == results.size()));
        results[id] = value;
        num_set++;
        if(num_set == results.size() && complete) {
            is_ready.notify_one();
        }
    }
    void set_complete() {
        threadns::unique_lock<boost::mutex> l(guard);
        complete = true;
        if(num_set == results.size()) {
            is_ready.notify_one();
        }
    }
    void wait() {
        threadns::unique_lock<boost::mutex> l(guard);
        while(!complete && num_set != results.size()) {
            is_ready.wait(l);
        }
    }
    T& get_result(int id) {
        threadns::unique_lock<boost::mutex> l(guard);
        return results[id];
    }
    size_t size() {
        threadns::unique_lock<boost::mutex> l(guard);
        return results.size();
    }
};
