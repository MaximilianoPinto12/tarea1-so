#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "jobs.h"
 
Job lista_jobs[MAX_JOBS];
 
//arreglo auxiliar de flags, avisa cuando un job terminó en background
static volatile sig_atomic_t jobs_terminados_flags[MAX_JOBS]= {0};
 
//se activa cada vez que un proceso hijo cambia de estado(finaliza)
void manejador_sigchld(int sig) {
    //evita advertencias de variable no usada
    (void)sig;
    //conserva errno para async-signal-safety
    int saved_errno = errno;
    int status;
    pid_t pid;
 
    //se llama a waitpid en un ciclo con WNOHANG.
    //como las señales POSIX no se encolan, un solo SIGCHLD puede significar que varios hijos terminaron simultáneamente
    while ((pid = waitpid(-1, &status, WNOHANG)) >0) {
        //marca el proceso correspondiente como terminado dentro del arreglo
        for (int i = 0; i <MAX_JOBS; i++) {
            if (lista_jobs[i].activo && lista_jobs[i].pid == pid) {
                //el flag se activa ANTES de bajar activo, asi nunca hay un
                //instante en que el job no este ni activo ni pendiente de aviso
                jobs_terminados_flags[i] = 1;
                lista_jobs[i].activo = 0;
                break;
            }
        }
    }
    //restaura el errno
    errno = saved_errno;
}
 
void inicializar_manejador_sigchld(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = manejador_sigchld;
    sigemptyset(&sa.sa_mask);
    
    //SA_RESTART reinicia automáticamente las llamadas a sistema interrumpidas
    //SA_NOCLDSTOP solo notifica si el hijo termina(no si se detiene con ctrl+z)
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
 
    if (sigaction(SIGCHLD, &sa, NULL) <0) {
        perror("sigaction(SIGCHLD)");
        exit(EXIT_FAILURE);
    }
}
 
int es_background(int *argc, char *argv[]) {
    if (*argc == 0) return 0;
 
    //comprueba si el último argumento es "&"
    if (strcmp(argv[*argc - 1], "&") == 0) {
        //remueve el token "&" de los argumentos del comando
        argv[*argc - 1] = NULL;
        (*argc)--;
        return 1;
    }
    return 0;
}
 
void agregar_job(pid_t pid, const char *comando) {
    //se bloquea SIGCHLD mientras se llena la casilla
    sigset_t bloqueo, anterior;
    sigemptyset(&bloqueo);
    sigaddset(&bloqueo, SIGCHLD);
    sigprocmask(SIG_BLOCK, &bloqueo, &anterior);
 
    for (int i = 0; i < MAX_JOBS; i++) {
        //una casilla solo se reutiliza si esta libre Y ya se aviso su "Done"
        //si no, se perderia el aviso y se mostraria el comando equivocado
        if (!lista_jobs[i].activo && !jobs_terminados_flags[i]){
            lista_jobs[i].id = i+1;
            lista_jobs[i].pid = pid;
            strncpy(lista_jobs[i].comando, comando, sizeof(lista_jobs[i].comando) - 1);
            //strncpy no garantiza el '\0' final
            lista_jobs[i].comando[sizeof(lista_jobs[i].comando) - 1] = '\0';
            //activo se escribe al final: recien ahi el manejador puede ver el job
            lista_jobs[i].activo = 1;
            int id = lista_jobs[i].id;
 
            sigprocmask(SIG_SETMASK, &anterior, NULL);
            printf("[%d] %d\n", id, (int)pid);
            fflush(stdout);
            return;
        }
    }
    sigprocmask(SIG_SETMASK, &anterior, NULL);
    fprintf(stderr, "miShell: límite máximo de trabajos en background alcanzado.\n");
}
 
void notificar_jobs_terminados(void) {
    for (int i = 0; i <MAX_JOBS; i++) {
        if (jobs_terminados_flags[i]) {
            printf("[%d]+ Done\t\t%s\n", i+1, lista_jobs[i].comando);
            //se limpia el flag DESPUES de imprimir: hasta ahora la casilla estaba reservada
            jobs_terminados_flags[i] =0;
        }
    }
    fflush(stdout);
}