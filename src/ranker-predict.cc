//#define _GNU_SOURCE
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <unordered_map>
#include <vector>
#include <string>

using namespace std;

int main(int argc, char** argv) {
    if(argc != 3) {
        fprintf(stderr, "USAGE: %s <model> <limit> < examples\n", argv[0]);
        return 1;
    }

    FILE* fp = fopen(argv[1], "r");
    if(!fp) {
      fprintf(stderr, "ERROR: cannot open \"%s\"\n", argv[1]);
      return 1;
    }

    int limit = atoi(argv[2]);
    fprintf(stderr, "limit: %d\n", limit);

    vector<double> weights;
    unordered_map<string, int> features;
    int buffer_size = 1024;
    char* buffer = (char*) malloc(buffer_size);

    int next_id = 0;
    while(NULL != fgets(buffer, buffer_size, fp)) {
        while(buffer[strlen(buffer) - 1] != '\n') {
            buffer_size *= 2;
            buffer = (char*) realloc(buffer, buffer_size);
            if(fgets(buffer + strlen(buffer), buffer_size - strlen(buffer), fp) == NULL) break;
        }
        buffer[strlen(buffer) - 1] = '\0'; // chop
        char* weight1 = strchr(buffer, ' ');
        *weight1 = '\0';
        char* weight2 = strchr(weight1 + 1, ' ');
        features[string(buffer)] = next_id;
        if(weight2 != NULL) {
            *weight2 = '\0';
            //weights.push_back(strtod(weight1 + 1, NULL));
            weights.push_back(strtod(weight2 + 1, NULL));
        } else {
            weights.push_back(strtod(weight1 + 1, NULL));
        }
        next_id++;
    }

    fprintf(stderr, "feature weight read successfully\n");

    fclose(fp);
    int num = 0;
    double avg_loss = 0;
    double one_best_loss = 0;
    double max = 0;
    double loss_of_max = 0;
    int is_one_best = 1;
    int argmax = 0;
    int current = 0;
    while(NULL != fgets(buffer, buffer_size, stdin)) {
        while(buffer[strlen(buffer) - 1] != '\n') {
            buffer_size *= 2;
            buffer = (char*) realloc(buffer, buffer_size);
            if(fgets(buffer + strlen(buffer), buffer_size - strlen(buffer), stdin) == NULL) break;
        }

        if(buffer[0] == '\n') {
            avg_loss += loss_of_max;
            if(num % 10 == 0) fprintf(stderr, "\r%d %f/%f", num, avg_loss / num, one_best_loss / num);
            num ++;
            fprintf(stdout, "%d\n", argmax);
            is_one_best = 1;
            current = 0;
            continue;
        }
	
	if(current == (limit))
	  continue;

        char* token;
        int first = 1;
        int label = 0;
        double score = 0.0;
        double loss = 0.0;
        for(token = strtok(buffer, " \t\n\r"); token != NULL; token = strtok(NULL, " \t\n\r")) {
            if(first == 1) {
                if(!strcmp(token, "1")) {
                    label = 1;
                }
                first = 0;
            } else {
                char* value_start = strrchr(token, ':');
                if(value_start != NULL) {
                    *value_start = '\0';
                    if(!strcmp(token, "nbe")) {
                        loss = strtod(value_start + 1, NULL);
                    } else {
                        string token_as_string = token;
                        unordered_map<string, int>::iterator found = features.find(token_as_string);
                        if(found != features.end()) {
                            double value = strtod(value_start + 1, NULL);
                            if(!isinf(value) && !isnan(value)) {
                                score += value * weights[(*found).second];
                            }
                        }
                    }
                }
            }
        }
        if(is_one_best) {
            max = score;
            loss_of_max = loss;
            one_best_loss += loss;
            is_one_best = 0;
            argmax = current;
        } else {
            if(score > max) {
                max = score;
                loss_of_max = loss;
                argmax = current;
            }
        }
        current++;
    }
    fprintf(stderr, "\r%d %f/%f\n", num, avg_loss / num, one_best_loss / num);
    return 0;
}

