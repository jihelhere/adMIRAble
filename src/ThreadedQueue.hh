#pragma once

#include <assert.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>

/* This class implements a blocking queue of jobs that are exectued by a
 * background thread.  Results should be stored in std::promise instances so
 * that you can retrieve them later.  It is mandatory to call drain() before
 * destroying the object so that the jobs are finished correctly and the thread
 * is terminated. Example:
 *
 * class MyQueue: public ThreadedQueue<std::pair<int,std::promise<int>*>> {
 *     public:
 *     MyQueue(): ThreadedQueue<std::pair<int,std::promise<int>*>>(100) { }
 *     void process(std::pair<int,std::promise<int>*>& i) {
 *         i.second->set_value(i.first);
 *     }
 * };
 *
 * int main(int argc, char** argv) {
 *     MyQueue queue;
 *     std::vector<std::promise<int>*> results;
 *     for(int i = 0; i < 200; i++) {
 *         results.push_back(new std::promise<int>());
 *         queue.enqueue(std::pair<int,std::promise<int>*>(i, results.back()));
 *     }
 *     for(auto i = results.begin(); i != results.end(); ++i) {
 *         fprintf(stdout, "%d\n", (*i)->get_future().get());
 *         delete (*i);
 *     }
 *     queue.drain();
 * }
 */

template<class T>
class ThreadedQueue {
    std::thread thread;
    std::mutex guard;
    std::condition_variable can_process;
    std::condition_variable can_enqueue;
    std::condition_variable queue_empty;
    int start, end, size, max_size;
    bool finished, running;
    std::queue<T> queue;

    public:

    // _max_size is the maximum size of the queue
    ThreadedQueue(int _max_size) : start(0), end(0), size(0), max_size(_max_size), finished(false), running(false) {
        thread = std::thread(&ThreadedQueue<T>::run, this);
    }

    // warning: you must call drain before destructing the object
    ~ThreadedQueue() {
    }

    // this is where you implement stuff
    virtual void process(T& item) = 0;

    void run() {
        running = true;
        while(true) {
            std::unique_lock<std::mutex> l(guard);
            while(size == 0) {
                if(finished) {
                    running = false;
                    queue_empty.notify_one();
                    return;
                }
                can_process.wait(l);
            }
            T item = queue.front();//[start];
            guard.unlock();
            process(item);
            guard.lock();
            queue.pop();
            start = (start + 1) % max_size;
            size--;
            can_enqueue.notify_one();
        }
    }

    // call enqueue with pairs of input and std::promise to be able to retrieve results
    void enqueue(T item) {
        std::unique_lock<std::mutex> l(guard);
        assert(!finished);
        while(size == max_size) can_enqueue.wait_for(l, std::chrono::duration<int,std::milli>(10));
        //queue[end] = item;
        queue.push(item);
        end = (end + 1) % max_size;
        size++;
        can_process.notify_one();
    }

    // finishes processing the queue
    void drain() {
        std::unique_lock<std::mutex> l(guard);
        if(!finished) {
            finished = true;
            while(running) {
                can_process.notify_all(); // unlock thread
                queue_empty.wait_for(l, std::chrono::duration<int,std::milli>(10)); // wait for it to finish
            }
            thread.join();
            while(thread.joinable()) sched_yield();
        }
    }
};
