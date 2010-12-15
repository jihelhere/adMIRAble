#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"

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
            hashtable_put(hashtable, buffer, (value_t) 1);
        }
    }
    fclose(fp);
    while(NULL != fgets(buffer, buffer_size, stdin)) {
        while(buffer[strlen(buffer) - 1] != '\n') {
            buffer_size *= 2;
            buffer = (char*) realloc(buffer, buffer_size);
            if(fgets(buffer + strlen(buffer), buffer_size - strlen(buffer), stdin) == NULL) break;
        }
        if(*buffer == '\n') {
            fprintf(stdout, "\n");
            continue;
        }
        char* token;
        int first = 1;
        for(token = strtok(buffer, " \t\n"); token != NULL; token = strtok(NULL, " \t\n")) {
            if(first) {
                first = 0;
                fprintf(stdout, "%s", token);
            }
            char* end = strrchr(token, ':');
            if(end != NULL) {
                *end = '\0';
                if(hashtable_get(hashtable, token).int_value != -1) {
                    *end = ':';
                    fprintf(stdout, " %s", token);
                }
            }
        }
        fprintf(stdout, "\n");
    }
    hashtable_free(hashtable);
    return 0;
}
