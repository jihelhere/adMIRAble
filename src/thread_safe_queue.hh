#pragma once
#include <mutex>
#include <atomic>
#include <condition_variable>

template<class C>
class thread_safe_queue{
    int start;
    int end;
    int length;
    int num;
    bool eof;
    C* array;

    std::condition_variable can_push;
    std::condition_variable can_pop;
    std::mutex guard;

    public:
    thread_safe_queue(int _length): start(0), end(0), length(_length), num(0), eof(false) {
        array = new C[length];
        for(int i = 0; i < length; i++) can_push.notify_one();
    }
    ~thread_safe_queue() {
        delete array;
    }
    void push(const C& c) {
        std::unique_lock<std::mutex> l(guard);
        while(num == length) can_push.wait(l);
        array[end] = c;
        end = (end + 1) % length;
        num++;
        can_pop.notify_one();
    }
    C pop() { 
        std::unique_lock<std::mutex> l(guard);
        while(num == 0) can_pop.wait(l);
        C output = array[start];
        start = (start + 1) % length;
        num--;
        can_push.notify_one();
        return output;
    }
    bool empty() {
        std::unique_lock<std::mutex> l(guard);
        return num == 0;
    }
    int size() {
        std::unique_lock<std::mutex> l(guard);
        return num;
    }
    void set_finished() {
        std::unique_lock<std::mutex> l(guard);
        eof = true;
    }
    bool finished() {
        std::unique_lock<std::mutex> l(guard);
        return eof && num == 0;
    }
};//thread_safe_queue
