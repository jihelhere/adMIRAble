#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"

int main(int argc, char** argv) {
    hashtable_t* global_counts = hashtable_new(10000000);
    hashtable_t* local_counts = hashtable_new(10000);
    int buffer_size = 1024;
    char* buffer = malloc(buffer_size);
    while(NULL != fgets(buffer, buffer_size, stdin)) {
        while(buffer[strlen(buffer) - 1] != '\n') {
            buffer_size *= 2;
            buffer = (char*) realloc(buffer, buffer_size);
            if(fgets(buffer + strlen(buffer), buffer_size - strlen(buffer), stdin) == NULL) break;
        }
        if(*buffer == '\n') {
            int i;
            for(i = 0; i < local_counts->num_values; i++) {
                if(local_counts->values[i].key != NULL) {
                    hashtable_inc(global_counts, local_counts->values[i].key, (value_t) 1, (value_t) 1);
                }
            }
            hashtable_free(local_counts);
            local_counts = hashtable_new(10000);
        } else {
            char* token;
            int first = 1;
            for(token = strtok(buffer, " \t"); token != NULL; token = strtok(NULL, " \t\n")) {
                if(first == 1) {
                    first = 0;
                } else {
                    char* name = strrchr(token, ':');
                    if(name != NULL) {
                        *name = '\0';
                        hashtable_inc(local_counts, token, (value_t) 1, (value_t) 1);
                    }
                }
            }
        }
    }
    free(buffer);
    int i;
    for(i = 0; i < global_counts->num_values; i++) {
        if(global_counts->values[i].key != NULL) {
            fprintf(stdout, "%s %d\n", global_counts->values[i].key, global_counts->values[i].value.int_value);
        }
    }
    hashtable_free(global_counts);
    hashtable_free(local_counts);
    return 0;
}
