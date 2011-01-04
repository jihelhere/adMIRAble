#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"
#include "vector.h"



int always_keep(char * token)
{
  return 
    !strcmp(token, "prob") || !strcmp(token, "nbe");
}


int main() {
    hashtable_t* hashtable = hashtable_new(1000000);
    int buffer_size = 1024;
    char* buffer = malloc(buffer_size);
    long int skipped = 0, total = 0;
    char** lines;
    vector_new(char*, lines, 20);
    while(NULL != fgets(buffer, buffer_size, stdin)) {
        while(buffer[strlen(buffer) - 1] != '\n') {
            buffer_size *= 2;
            buffer = (char*) realloc(buffer, buffer_size);
            if(fgets(buffer + strlen(buffer), buffer_size - strlen(buffer), stdin) == NULL) break;
        }
        if(buffer[0] == '\n') {
            int i;
            for(i = 0; i < vector_length(lines); i++) {
                char* token;
                int first = 1;
                for(token = strtok(lines[i], " \t\n"); token != NULL; token = strtok(NULL, " \t\n")) {
                    if(first == 1) {
                        fprintf(stdout, "%s", token);
                        first = 0;
                    } else {
                        char* end = strrchr(token, ':');
                        if(end != NULL) {
                            *end = '\0';
			    if(always_keep(token)) {
			      *end = ':';
			      fprintf(stdout, " %s", token);
			    }
			    else {
			      *end = ':';
			      int count = hashtable_get(hashtable, token).int_value;
			      if(count != vector_length(lines)) {
				fprintf(stdout, " %s", token);
			      }
			      else 
				++skipped;
			    }
			}
			++total;
                    }
                }
                fprintf(stdout, "\n");
                free(lines[i]);
            }
            vector_length(lines) = 0;
            hashtable_free(hashtable);
            hashtable = hashtable_new(10000);
            fprintf(stdout, "\n");
        } else {
            vector_push(lines, strdup(buffer));
            char* token;
            int first = 1;
            for(token = strtok(buffer, " \t"); token != NULL; token = strtok(NULL, " \t\n")) {
                if(first == 1) {
                    first = 0;
                } else {
                    /* char* end = strrchr(token, ':'); */
                    /* if(end != NULL) { */
                    /*     *end = '\0'; */
                        hashtable_inc(hashtable, token, (value_t) 1, (value_t) 1);
			/* } */
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
    vector_free(lines);
    fprintf(stderr, "skipped %ld/%ld features (%.2f%%)\n", skipped, total, 100.0 * skipped / (double) total);
    return 0;
}
