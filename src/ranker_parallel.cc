#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ranker.hh"
#include "thread_safe_queue.hh"
#include <thread>
#include <future>
#include <utility>
#include <vector>
#include <string>

class input_thread {
    thread_safe_queue<std::pair<char*,std::promise<double>*>>* queue;
    ranker::predictor& model;
    std::thread thread;

    public:
    void push(std::pair<char*,std::promise<double>*> item) {
        queue->push(item);
    }
    void start() {
        while(!queue->finished()) {
            if(!queue->empty()) {
                std::pair<char*,std::promise<double>*> input = queue->pop();
                ranker::Example x;
                char *inputstring = input.first;
                char *token = NULL; 
                token =  strsep(&inputstring, " \t"); // skip label
                for(;(token = strsep(&inputstring, " \t\n"));) {
                    if(!strcmp(token,"")) continue;
                    char* value = strrchr(token, ':');
                    if(value != NULL) {
                        *value = '\0';
                        double value_as_double = strtod(value + 1, NULL);
                        //nbe is the loss, not a feature
                        if(!strcmp(token, "nbe")) {
                            x.loss = value_as_double;
                        } else {
                            int location = strtol(token, NULL, 10);
                            x.features.push_back(ranker::feature(location, value_as_double));
                        }
                    }
                }
                double score = model.compute_score(x);
                input.second->set_value(score);
                free(input.first);
            } else {
                sched_yield();
            }
        }
    }

    void stop() {
        queue->set_finished();
        thread.join();
    }

    input_thread(ranker::predictor& _model, int queue_size):model(_model) {
        queue = new thread_safe_queue<std::pair<char*,std::promise<double>*>>(queue_size);
        thread = std::thread(&input_thread::start, this);
    }
    ~input_thread() {
        delete queue;
    }
};

class output_thread {
    std::thread thread;
    thread_safe_queue<std::vector<std::promise<double>*>*>* queue;

    public:
    void push(std::vector<std::promise<double>*>* item) {
        queue->push(item);
    }
    void start() {
        while(!queue->finished()) {
            if(!queue->empty() && !queue->finished()) {
                std::vector<std::promise<double>*>* result = queue->pop(); // single consumer
                int argmax = -1;
                double max = 0;
                for(size_t i = 0; i < result->size(); i++) {
                    double value = (*result)[i]->get_future().get();
                    if(argmax == -1 || value > max) {
                        argmax = i;
                        max = value;
                    }
                    delete (*result)[i];
                }
                delete result;
                fprintf(stdout, "%d\n", argmax);
            } else {
                sched_yield();
            }
        }
    }

    void stop() {
        queue->set_finished();
        thread.join();
    }

    output_thread(int queue_size) {
        queue = new thread_safe_queue<std::vector<std::promise<double>*>*>(queue_size);
        thread = std::thread(&output_thread::start, this);
    }
    ~output_thread() {
        delete queue;
    }
};

int main(int argc, char** argv) {
    if(argc < 3 || argc > 4) {
        fprintf(stderr, "usage: %s <num-threads> <model> [num-candidates]\n", argv[0]);
        return 1;
    }
    int num_threads = strtol(argv[1], NULL, 10);
    int num_candidates = -1;
    if(argc == 4) num_candidates = strtol(argv[3], NULL, 10);
    ranker::predictor model(1, std::string(argv[2]));
    output_thread output(200);

    std::vector<input_thread*> input;
    int current_input_thread = 0;
    for(int i = 0; i < num_threads; i++) {
        input.push_back(new input_thread(model, 200));
    }

    char* buffer = NULL;
    size_t buffer_length = 0;
    ssize_t length = 0;

    std::vector<std::promise<double>*>* results = new std::vector<std::promise<double>*>();
    while(0 <= (length = getline(&buffer, &buffer_length, stdin))) {
        if(length == 1) {
            output.push(results);
            results = new std::vector<std::promise<double>*>();
        } else if(num_candidates == -1 || (int) results->size() < num_candidates) {
            std::promise<double>* result = new std::promise<double>();
            results->push_back(result);
            input[current_input_thread]->push(std::pair<char*,std::promise<double>*>(strdup(buffer), results->back()));
            current_input_thread++;
            if(current_input_thread >= num_threads) current_input_thread = 0; // round-robin
        }
    }
    for(int i = 0; i < num_threads; i++) {
        input[i]->stop();
        delete input[i];
    }
    output.stop();
    return 0;
}
