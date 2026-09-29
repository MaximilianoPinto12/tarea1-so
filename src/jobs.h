#ifndef JOBS_H
#define JOBS_H
 
#include <sys/types.h>
#include <signal.h>
 
#define MAX_JOBS 64
 
typedef struct {
    int id;
    pid_t pid;
    char comando[256];
    volatile sig_atomic_t activo;
    unsigned long tiempo_anterior;
} Job;
 
extern Job lista_jobs[MAX_JOBS];
 
void inicializar_manejador_sigchld(void);
 
int es_background(int *argc, char *argv[]);
 
void agregar_job(pid_t pid, const char *comando);
 
void notificar_jobs_terminados(void);
 
#endif
 