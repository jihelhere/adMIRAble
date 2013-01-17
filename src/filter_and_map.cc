#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unordered_map>

#include "utils.h"


#include <algorithm>

#define MAX_FEATURE 10000


using namespace std;

struct feature_t {
  int id;
  char* name;
  char* value;

  bool operator<(const feature_t& other) const
  {
    return id < other.id;
  }

} ;


int main(int argc, char** argv) {
    if(argc != 3) {
        fprintf(stderr, "USAGE: %s <counts> <cutoff>\n", argv[0]);
        exit(1);
    }
    unordered_map<string, int> dictionary(10000000);
    int cutoff = atoi(argv[2]);
    FILE* fp = fopen(argv[1], "r");
    if(fp == NULL) {
        perror("dictionary");
        exit(1);
    }
    size_t buffer_size = 0;
    char* buffer = NULL;
    int length = 0;
    int num = 1;
    while(0 < (length = read_line(&buffer, &buffer_size, fp))) {
        if(*buffer == '\n') continue;
        buffer[length - 1] = '\0'; // chop
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
    feature_t features[MAX_FEATURE];
    int num_features = 0;
    while(0 < (length = read_line(&buffer, &buffer_size, stdin))) {
        buffer[length - 1] = '\0'; // chop
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
                auto id = dictionary.find(string(token));
                if(id != dictionary.end()) {
                    if(num_features >= MAX_FEATURE) {
                      fprintf(stderr, "ERROR: example has more than %d features\n", MAX_FEATURE);
                        exit(1);
                    }

                    feature_t& current = features[num_features];

                    current.id = id->second;
                    current.value = end + 1;
                    current.name = token;
                    ++num_features;
                }
            }
        }

        std::sort(features, features+num_features);

        for(int i = 0; i < num_features; i++) {
            fprintf(stdout, " %d:%s", features[i].id, features[i].value);
        }
        fprintf(stdout, "\n");
    }
    return 0;
}
