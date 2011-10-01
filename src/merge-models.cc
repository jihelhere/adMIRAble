#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <unordered_map>
#include "utils.h"

using namespace std;

int main(int argc, char** argv) {
    if(argc == 1) {
        fprintf(stderr, "USAGE: %s <model1> <model2> <model3> ...\n", argv[0]);
        exit(1);
    }
    unordered_map<string, double> model;
    size_t buffer_size = 0;
    char* buffer = NULL;
    int i;
    for(i = 1; i < argc; i++) {
        FILE* input = open_pipe(argv[i], "r");
        while(0 < read_line(&buffer, &buffer_size, input)) {
            char* value = strchr(buffer, ' ');
            *value = '\0';
            string name = buffer;
            if(value != NULL) {
                double weight = strtod(value + 1, NULL);
                unordered_map<string, double>::iterator found = model.find(name);
                if(found != model.end()) {
                    (*found).second += weight;
                } else {
                    model[name] = weight;
                }
            }
        }
        close_pipe(input);
    }
    for(unordered_map<string, double>::iterator current = model.begin(); current != model.end(); current++) {
        fprintf(stdout, "%s %32.31g\n", (*current).first.c_str(), (*current).second / (argc - 1));
    }
    return 0;
}
