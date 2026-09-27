#ifndef BUILTINS_H
#define BUILTINS_H

typedef struct {
    int id;
    pid_t pid;
    char comando[256];
    char estado[32]; 
} Job;

#define MAX_JOBS 64

int ejecutar_builtin(int argc, char *argv[], Job jobs[], int *num_jobs);

#endif