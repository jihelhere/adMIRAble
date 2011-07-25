#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ranker.hh"

int main(int argc, char** argv) {
    if(argc < 2) {
        fprintf(stderr, "usage: %s <model>\n", argv[0]);
        return 1;
    }
    ranker::predictor model(1, std::string(argv[1]));

    char* buffer = NULL;
    size_t buffer_length = 0;
    ssize_t length = 0;

    std::vector<ranker::example> examples;
    while(0 <= (length = getline(&buffer, &buffer_length, stdin))) {
        if(length == 1) {
            fprintf(stdout, "%d\n", model.predict(examples));
            examples.clear();
        } else {
            ranker::example x;
            char *inputstring = buffer;
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
            examples.push_back(x);
        }
    }
    return 0;
}
