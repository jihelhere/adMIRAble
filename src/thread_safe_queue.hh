#pragma once
#include <queue>
#include <mutex>

template<class C>
class thread_safe_queue{
    std::queue<C> queue;
    std::mutex guard;

    public:
    thread_safe_queue() {
    }
    ~thread_safe_queue() {
    }
    void push(const C& c) {
        std::lock_guard<std::mutex> l(guard); 
        queue.push(c);
    }
    void pop() { 
        std::lock_guard<std::mutex> l(guard); 
        queue.pop(); 
    }
    C atomic_pop() {
        std::lock_guard<std::mutex> l(guard); 
        C output = queue.front();
        queue.pop();
        return output;
    }
    bool empty() { 
        std::lock_guard<std::mutex> l(guard); 
        return queue.empty(); 
    }
    C& front() { 
        std::lock_guard<std::mutex> l(guard); 
        return queue.front(); 
    }
    const C& front() const { 
        std::lock_guard<std::mutex> l(guard); 
        return queue.front(); 
    }
    C& back() { 
        std::lock_guard<std::mutex> l(guard); 
        return queue.back(); 
    }
    const C& back() const { 
        std::lock_guard<std::mutex> l(guard); 
        return queue.back(); 
    }
    size_t size() const {
        std::lock_guard<std::mutex> l(guard);
        return queue.size();
    }
};//thread_safe_queue
