#pragma once

#ifdef USE_BOOST_THREAD
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#else
#include <mutex>
#include <condition_variable>
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
#ifdef USE_BOOST_THREAD
    boost::mutex guard;
    boost::condition_variable is_ready;
#else
    std::mutex guard;
    std::condition_variable is_ready;
#endif
    std::vector<T> results;
    size_t num_set;
    bool complete;
    public:
    ResultVector(): num_set(0), complete(false) {
    }
    int add_result() {
#ifdef USE_BOOST_THREAD
        boost::unique_lock<boost::mutex> l(guard);
#else
        std::unique_lock<std::mutex> l(guard);˙
#endif
        results.push_back(T());
        return results.size() - 1;
    }
    void set_result(int id, T& value) {
#ifdef USE_BOOST_THREAD
        boost::unique_lock<boost::mutex> l(guard);
#else
        std::unique_lock<std::mutex> l(guard);˙
#endif
        assert(!(complete && num_set == results.size()));
        results[id] = value;
        num_set++;
        if(num_set == results.size() && complete) {
            is_ready.notify_one();
        }
    }
    void set_complete() {
#ifdef USE_BOOST_THREAD
        boost::unique_lock<boost::mutex> l(guard);
#else
        std::unique_lock<std::mutex> l(guard);˙
#endif
        complete = true;
        if(num_set == results.size()) {
            is_ready.notify_one();
        }
    }
    void wait() {
#ifdef USE_BOOST_THREAD
        boost::unique_lock<boost::mutex> l(guard);
#else
        std::unique_lock<std::mutex> l(guard);˙
#endif
        while(!complete && num_set != results.size()) {
            is_ready.wait(l);
        }
    }
    T& get_result(int id) {
#ifdef USE_BOOST_THREAD
        boost::unique_lock<boost::mutex> l(guard);
#else
        std::unique_lock<std::mutex> l(guard);˙
#endif
        return results[id];
    }
    size_t size() {
#ifdef USE_BOOST_THREAD
        boost::unique_lock<boost::mutex> l(guard);
#else
        std::unique_lock<std::mutex> l(guard);˙
#endif
        return results.size();
    }
};
