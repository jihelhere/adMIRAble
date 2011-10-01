#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Predictor.hh"

int main(int argc, char** argv) {
    if(argc < 2 || argc > 3) {
        fprintf(stderr, "usage: %s <model> [num-candidates]\n", argv[0]);
        return 1;
    }
    ranker::Predictor model(1, std::string(argv[1]));
    int num_candidates = -1;
    if(argc == 3) num_candidates = strtol(argv[2], NULL, 10);

    char* buffer = NULL;
    size_t buffer_length = 0;
    ssize_t length = 0;

    std::vector<ranker::Example> examples;
    while(0 <= (length = read_line(&buffer, &buffer_length, stdin))) {
        if(length == 1) {
            fprintf(stdout, "%d\n", model.predict(examples));
            examples.clear();
        } else if(num_candidates == -1 || (int) examples.size() < num_candidates) {
            ranker::Example x(buffer);
            examples.push_back(x);
        }
    }
    return 0;
}
