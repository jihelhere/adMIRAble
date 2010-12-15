#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main(int argc, char** argv) {
    int buffer_size = 1024;
    char* buffer = malloc(buffer_size);
    while(NULL != fgets(buffer, buffer_size, stdin)) {
        while(buffer[strlen(buffer) - 1] != '\n') {
            buffer_size *= 2;
            buffer = (char*) realloc(buffer, buffer_size);
            if(fgets(buffer + strlen(buffer), buffer_size - strlen(buffer), stdin) == NULL) break;
        }
        char* start = strstr(buffer, "logprob:");
        if(start != NULL) {
            char* value_start = start + 8;
            char* end = value_start;
            while(*end != '\0' && *end != ' ' && *end != '\t' && *end != '\r' && *end != '\n') end++;
            double value = strtod(value_start, NULL);
            /*if(isnan(value) || isinf(value)) value = 0;
            if(value < -500) value = -500;
            value = (value + 500) / 500;*/
            value = exp(value);
            *start = '\0';
            fprintf(stdout, "%sprob:%lg%s", buffer, value, end);
        } else {
            fprintf(stdout, "%s", buffer);
        }
    }
    free(buffer);
    return 0;
}
