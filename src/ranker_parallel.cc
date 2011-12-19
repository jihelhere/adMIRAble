#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Predictor.hh"
#include "ThreadedQueue.hh"
#include "ResultVector.hh"
// #include <thread>
// #include <future>
#include <utility>
#include <vector>
#include <string>

#include "utils.h"


// warning: had to handle all memory allocations/deallocations within threads

class input_thread: public ThreadedQueue<std::pair<std::string,Result<double>>> {
    ranker::Predictor& model;

    public:
    void process(std::pair<std::string,Result<double>>& input) {
      char *copy = strdup(input.first.c_str());

        ranker::Example x(copy);
	free(copy);
        double score = model.compute_score(x);
        input.second.set_result(score);
    }

    input_thread(ranker::Predictor& _model, int queue_size):ThreadedQueue<std::pair<std::string,Result<double>>>(queue_size), model(_model) { }
};

class output_thread : public ThreadedQueue<ResultVector<double>*> {
    public:
    void process(ResultVector<double>*& result) {
        result->wait();
        int argmax = -1;
        double max = 0;
        for(size_t i = 0; i < result->size(); i++) {
            double value = result->get_result(i);
            if(argmax == -1 || value > max) {
                argmax = i;
                max = value;
            }
        }
        //delete result;
        fprintf(stdout, "%d\n", argmax);
    }
    ResultVector<double>* new_result() {
        return new ResultVector<double>();
    }

    output_thread(int queue_size) : ThreadedQueue<ResultVector<double>*>(queue_size) { }
};

int main(int argc, char** argv) {
    if(argc < 3 || argc > 4) {
        fprintf(stderr, "usage: %s <num-threads> <model> [num-candidates]\n", argv[0]);
        return 1;
    }
    int num_threads = strtol(argv[1], NULL, 10);
    int num_candidates = -1;
    if(argc == 4) num_candidates = strtol(argv[3], NULL, 10);
    ranker::Predictor model(1, std::string(argv[2]));
    output_thread output(num_threads);

    std::vector<input_thread*> input;
    int current_input_thread = 0;
    for(int i = 0; i < num_threads; i++) {
        input.push_back(new input_thread(model, 10));
    }

    char* buffer = NULL;
    size_t buffer_length = 0;
    ssize_t length = 0;

    std::vector<ResultVector<double>*> results;
    results.push_back(new ResultVector<double>());
    while(0 <= (length = read_line(&buffer, &buffer_length, stdin))) {
        if(length == 1) {
            results.back()->set_complete();
            output.enqueue(results.back());
            results.push_back(new ResultVector<double>());
        } else if(num_candidates == -1 || (int) results.back()->size() < num_candidates) {
            int id = results.back()->add_result();
            input[current_input_thread]->enqueue(std::pair<std::string,Result<double>>(std::string(buffer), Result<double>(id, results.back())));
            current_input_thread++;
            if(current_input_thread >= num_threads) current_input_thread = 0; // round-robin
        }
    }
    for(int i = 0; i < num_threads; i++) {
        input[i]->drain();
        delete input[i];
    }
    output.drain();
    for(auto i = results.begin(); i != results.end(); ++i) {
        delete (*i);
    }
    return 0;
}
