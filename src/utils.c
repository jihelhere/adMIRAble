#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "utils.h"

#define MODE_READ 1
#define MODE_WRITE 2

#define INIT_BUFFER_SIZE 512

typedef struct pipe_info {
    char* extension;
    int extension_length;
    char* read_command;
    char* write_command;
} pipe_info_t;

int __num_info = 2;
pipe_info_t __info[2] = {
    {
        .extension = ".gz",
        .extension_length = 3,
        .read_command = "zcat",
        .write_command = "gzip"
    },
    {
        .extension = ".bz2",
        .extension_length = 4,
        .read_command = "bzcat",
        .write_command = "bzip2"
    }
};

FILE* open_pipe(const char* filename, const char* mode_string) {
    const char* command = NULL;
    int length = strlen(filename);
    int mode = 0;
    if(strcmp(mode_string, "r") == 0) {
        mode = MODE_READ;
    } else if(strcmp(mode_string, "w") == 0) {
        mode = MODE_WRITE;
    } else {
        fprintf(stderr, "ERROR: unsupported mode \"%s\" when opening \"%s\"\n", mode_string, filename);
        return NULL;
    }
    int i;
    for(i = 0; i < __num_info; i++) {
        if(strcmp(filename + length - __info[i].extension_length, __info[i].extension) == 0) {
            if(mode == MODE_READ) command = __info[i].read_command;
            else if(mode == MODE_WRITE) command = __info[i].write_command;
            break;
        }
    }
    if(command == NULL) {
        return fopen(filename, mode_string);
    }
    /*if(command != NULL && mode == MODE_READ) fprintf(stderr, "%s < %s |\n", command, filename);
    if(command != NULL && mode == MODE_WRITE) fprintf(stderr, "| %s > %s\n", command, filename);*/
    int pipe_fd[2];
    pid_t childpid;
    if(pipe(pipe_fd) == -1) {
        perror("openpipe/pipe");
        exit(1);
    }
    if((childpid = fork()) == -1) {
        perror("openpipe/fork");
        exit(1);
    }
    if(childpid == 0) {
        if(mode == MODE_READ) {
            close(pipe_fd[0]);
            int fd = open(filename, O_RDONLY);
            if(fd == -1) {
                perror(filename);
                exit(1);
            }
            fclose(stdout);
            if(dup2(pipe_fd[1], STDOUT_FILENO) == -1) perror("openpipe/stdout");
            fclose(stdin);
            if(dup2(fd, STDIN_FILENO) == -1) perror("openpipe/stdin");
        } else if(mode == MODE_WRITE) {
            close(pipe_fd[1]);
            int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0644);
            if(fd == -1) {
                perror(filename);
                exit(1);
            }
            if(dup2(pipe_fd[0], STDIN_FILENO) == -1) perror("openpipe/stdin");
            if(dup2(fd, STDOUT_FILENO) == -1) perror("openpipe/stdout");
        }
        execlp(command, command, (char*) NULL);
        perror("openpipe/execl");
        exit(1);
    }
    if(mode == MODE_WRITE) {
        close(pipe_fd[0]);
        return fdopen(pipe_fd[1], "w");
    } else if(mode == MODE_READ) {
        close(pipe_fd[1]);
        return fdopen(pipe_fd[0], "r");
    }
    return NULL;
}

int close_pipe(FILE* fp) {
    fclose(fp);
    int status;
    wait(&status);
    return status;
}


int read_line(char** buffer, size_t* buffer_size, FILE* fp)
{
  if(*buffer == NULL && *buffer_size == 0) {
    *buffer_size = INIT_BUFFER_SIZE;
    *buffer = (char*) malloc(INIT_BUFFER_SIZE*sizeof(char));
  }

  if(NULL == fgets(*buffer, *buffer_size, fp)) {
    return -1;
  }

  size_t l = strlen(*buffer);
  while((*buffer)[l - 1] != '\n') {
    *buffer_size *= 2;
    *buffer = (char*) realloc(*buffer, *buffer_size);
    if(fgets(*buffer + l, *buffer_size - l, fp) == NULL) return -1;
    l = strlen(*buffer);
  }

  return l;
}


