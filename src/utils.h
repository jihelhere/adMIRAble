#ifndef __OPENPIPE_H__
#define __OPENPIPE_H__
#endif

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

FILE* open_pipe(const char* filename, const char* mode);
int close_pipe(FILE* fp);
int read_line(char** buffer, int* buffer_size, FILE* fp);

#ifdef __cplusplus
}
#endif
