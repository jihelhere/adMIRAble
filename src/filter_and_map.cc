#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unordered_map>

using namespace std;

typedef struct feature {
  int id;
  char* name;
  char* value;
} feature_t;

int feature_id_comparator(const void* a, const void* b) {
    if(((feature_t*)a)->id > ((feature_t*)b)->id) return 1;
    return -1;
}

bool dictionary_comparator(const pair<string, int> &a, const pair<string, int> &b) {
    return a.first < b.first;
}

struct eqstr
{
    bool operator()(const char* s1, const char* s2) const
    {
        return (s1 == s2) || (s1 && s2 && strcmp(s1, s2) == 0);
    }
};

int main(int argc, char** argv) {
    if(argc != 3) {
        fprintf(stderr, "USAGE: %s <counts> <cutoff>\n", argv[0]);
        exit(1);
    }
    unordered_map<string, int> dictionary;
    int cutoff = atoi(argv[2]);
    FILE* fp = fopen(argv[1], "r");
    if(fp == NULL) {
        perror("dictionary");
        exit(1);
    }
    int buffer_size = 1024;
    char* buffer = (char*) malloc(buffer_size);
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
            dictionary[strdup(buffer)] = num;
            num ++;
            if(num % 100000 == 0) fprintf(stderr, "\rloading %d", num);
        }
    }
    fprintf(stderr, "\rloading %d\n", num);
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
                unordered_map<string, int>::const_iterator id = dictionary.find(string(token));
                if(id != dictionary.end()) {
                    if(num_features >= 100000) {
                        fprintf(stderr, "ERROR: example has more than 100k features\n");
                        exit(1);
                    }
                    features[num_features].id = id->second;
                    features[num_features].value = end + 1;
                    features[num_features].name = token;
                    num_features++;
                }
            }
        }
        qsort(features, num_features, sizeof(feature_t), feature_id_comparator);
        int i;
        for(i = 0; i < num_features; i++) {
            if(!strcmp(features[i].name, "nbe")) {
                fprintf(stdout, " nbe:%s", features[i].value);
            } else {
                fprintf(stdout, " %d:%s", features[i].id, features[i].value);
            }
        }
        fprintf(stdout, "\n");
    }
    return 0;
}