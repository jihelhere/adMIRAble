#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"

int main(int argc, char** argv) {
    hashtable_t* hashtable = hashtable_new(10000000);
    int buffer_size = 1024;
    char* buffer = malloc(buffer_size);
    while(NULL != fgets(buffer, buffer_size, stdin)) {
        while(buffer[strlen(buffer) - 1] != '\n') {
            buffer_size *= 2;
            buffer = (char*) realloc(buffer, buffer_size);
            if(fgets(buffer + strlen(buffer), buffer_size - strlen(buffer), stdin) == NULL) break;
        }
        char* token;
        int first = 1;
        for(token = strtok(buffer, " \t\n\r"); token != NULL; token = strtok(NULL, " \t\n\r")) {
            if(first == 1) {
                first = 0;
            } else {
                char* name = strrchr(token, ':');
                if(name != NULL) {
                    *name = '\0';
                    hashtable_inc(hashtable, token, (value_t) 1, (value_t) 1);
                }
            }
        }
    }
    free(buffer);
    int i;
    for(i = 0; i < hashtable->num_values; i++) {
        if(hashtable->values[i].key != NULL) {
            fprintf(stdout, "%s %d\n", hashtable->values[i].key, hashtable->values[i].value.int_value);
        }
    }
    hashtable_free(hashtable);
    return 0;
}
