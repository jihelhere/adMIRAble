#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

int main(int argc, char** argv) {
    FILE* input = open_pipe("foo.gz", "r");
    if(input == NULL) {
        perror("input");
        return 1;
    }
    FILE* output = open_pipe("bar.bz2", "w");
    if(output == NULL) {
        perror("output");
        return 1;
    }
    int buffer_size = 1024;
    char* buffer = (char*) malloc(buffer_size);
    while(read_example_line(&buffer, &buffer_size, input)) {
        fprintf(output, "%s", buffer);
    }
    close_pipe(output);
    close_pipe(input);
    return 0;
}
