#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include "utils.h"

int compute_num_examples(char* filename) {
    int num = 0;
    FILE* fp = open_pipe(filename, "r");
    if(!fp) return 0;

    char previous = '\0';
    while(previous != EOF) {
        char current = getc(fp);
        if(feof(fp)) break;
        if(previous == '\n' && current == '\n') 
            ++num;
        previous = current;
    }
    close_pipe(fp);
    return num;
}

int main(int argc, char** argv) {
    if(argc != 4) {
        fprintf(stderr, "USAGE: %s <training-set> <num-shards> <shard-stem>\n", argv[0]);
        exit(1);
    }
    int num_examples = compute_num_examples(argv[1]);
    int shard_size = (num_examples / atoi(argv[2])) + 1;
    fprintf(stdout, "num examples: %d, shard size: %d\n", num_examples, shard_size);

    FILE* input = open_pipe(argv[1], "r");
    int buffer_size = 1024;
    char* buffer = (char*) malloc(buffer_size);
    int num_output = 0;
    int current_shard = 1;
    char* output_filename;
    asprintf(&output_filename, "%s.%d", argv[3], current_shard);
    FILE* output = open_pipe(output_filename, "w");
    std::vector<char*> lines;
    while(read_example_line(&buffer, &buffer_size, input)) {
        if(buffer[0] == '\n') {
            int i;
            for(i = 0; i < lines.size(); ++i) {
                fprintf(output, "%s", lines[i]);
                free(lines[i]);
            }
            fprintf(output, "\n");
            lines.clear();
            ++num_output;
            if(num_output >= shard_size) {
                close_pipe(output);
                ++current_shard;
                asprintf(&output_filename, "%s.%d", argv[3], current_shard);
                output = open_pipe(output_filename, "w");
                num_output = 0;
            }
        } else {
            lines.push_back(strdup(buffer));
        }
    }
    if(lines.size() > 0) {
        int i;
        for(i = 0; i < lines.size(); ++i) {
            fprintf(output, "%s", lines[i]);
            free(lines[i]);
        }
        fprintf(output, "\n");
    }
    close_pipe(output);
    close_pipe(input);
}
