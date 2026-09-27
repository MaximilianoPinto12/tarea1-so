#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include "executor.h"

int ejecutar_foreground(char *argv[]) {
    if (argv== NULL || argv[0]==NULL) {
        return 0;
    }

    //crea un proceso hijo
    pid_t pid = fork();

    if (pid < 0) {
        //error al crear el proceso hijo
        perror("fork");
        return -1;
    } 
    else if (pid == 0) {
        //proceso hijo

        //reemplaza la imagen del proceso hijo por el programa solicitado.
        // execvp busca el ejecutable en la variable de entorno PATH si no tiene '/'
        execvp(argv[0], argv);

        //si execvp retorna, significa que ocurrió un error
        perror("execvp");
        
        //finaliza el proceso inmediatamente enviando un código de error
        _exit(127);
    } 
    else {
        //proceso padre (shell)
        int status;

        //la shell se bloquea y espera a que el hijo específico (pid) termine
        //waitpid devuelve cuando el hijo cambia de estado (finaliza)
        if (waitpid(pid, &status, 0) < 0) {
            //manejo del caso si waitpid es interrumpido por una señal
            if (errno != EINTR) {
                perror("waitpid");
            }
        }
    }

    return 0;
}