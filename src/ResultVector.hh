#include <mutex>
#include <condition_variable>

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
    std::mutex guard;
    std::condition_variable is_ready;
    std::vector<T> results;
    size_t num_set;
    bool complete;
    public:
    ResultVector(): num_set(0), complete(false) {
    }
    int add_result() {
        std::unique_lock<std::mutex> l(guard);
        results.push_back(T());
        return results.size() - 1;
    }
    void set_result(int id, T& value) {
        std::unique_lock<std::mutex> l(guard);
        assert(!(complete && num_set == results.size()));
        results[id] = value;
        num_set++;
        if(num_set == results.size() && complete) {
            is_ready.notify_one();
        }
    }
    void set_complete() {
        std::unique_lock<std::mutex> l(guard);
        complete = true;
        if(num_set == results.size()) {
            is_ready.notify_one();
        }
    }
    void wait() {
        std::unique_lock<std::mutex> l(guard);
        while(!complete && num_set != results.size()) {
            is_ready.wait(l);
        }
    }
    T& get_result(int id) {
        std::unique_lock<std::mutex> l(guard);
        return results[id];
    }
    size_t size() {
        std::unique_lock<std::mutex> l(guard);
        return results.size();
    }
};
