#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"
#include "vector.h"

typedef struct feature {
  int id;
  char* value;
} feature_t;

int feature_id_comparator(const void* a, const void* b) {
    if(((feature_t*)a)->id > ((feature_t*)b)->id) return 1;
    return -1;
}

int main(int argc, char** argv) {
    if(argc != 3) {
        fprintf(stderr, "USAGE: %s <counts> <cutoff>\n", argv[0]);
        exit(1);
    }
    hashtable_t* hashtable = hashtable_new(100000000);
    int cutoff = atoi(argv[2]);
    FILE* fp = fopen(argv[1], "r");
    if(fp == NULL) {
        perror("dictionary");
        exit(1);
    }
    int buffer_size = 1024;
    char* buffer = malloc(buffer_size);
    int num = 1;
    while(NULL != fgets(buffer, buffer_size, fp)) {
        while(buffer[strlen(buffer) - 1] != '\n') {
            buffer_size *= 2;
            buffer = (char*) realloc(buffer, buffer_size);
            if(fgets(buffer + strlen(buffer), buffer_size - strlen(buffer), stdin) == NULL) break;
        }
        if(*buffer == '\n') continue;
        buffer[strlen(buffer) - 1] = '\0'; // chop
        char* number = strchr(buffer, ' ');
        *number = '\0';
        number++;
        int value = strtol(number, NULL, 10);
        if(value >= cutoff) {
            hashtable_put(hashtable, buffer, (value_t) num);
            num ++;
        }
    }
    fclose(fp);
    feature_t features[100000];
    int num_features = 0;
    while(NULL != fgets(buffer, buffer_size, stdin)) {
        while(buffer[strlen(buffer) - 1] != '\n') {
            buffer_size *= 2;
            buffer = (char*) realloc(buffer, buffer_size);
            if(fgets(buffer + strlen(buffer), buffer_size - strlen(buffer), stdin) == NULL) break;
        }
        buffer[strlen(buffer) - 1] = '\0'; // chop
        char* token;
        int first = 1;
        num_features = 0;
        for(token = strtok(buffer, " \t\n"); token != NULL; token = strtok(NULL, " \t\n")) {
            if(first) {
                first = 0;
                fprintf(stdout, "%s", token);
            }
            char* end = strrchr(token, ':');
            if(end != NULL) {
                *end = '\0';
                int id = hashtable_get(hashtable, token).int_value;
                if(id != -1) {
                    features[num_features].id = id;
                    features[num_features].value = end + 1;
                    num_features++;
                }
            }
        }
        qsort(features, num_features, sizeof(feature_t), feature_id_comparator);
        int i;
        for(i = 0; i < num_features; i++) {
            fprintf(stdout, " %d:%s", features[i].id, features[i].value);
        }
        fprintf(stdout, "\n");
    }
    hashtable_free(hashtable);
    return 0;
}
