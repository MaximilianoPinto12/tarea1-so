#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include "executor.h"
#include "jobs.h"

int ejecutar_comando(char *argv[], int en_background) {
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
        if (en_background) {
            //no se bloquea con waitpid(), registra el job en la lista
            agregar_job(pid, argv[0]);
        }
        else {
            int status;
            //la shell se bloquea y espera a que el hijo específico (pid) termine
            //waitpid devuelve cuando el hijo cambia de estado (finaliza)
            while (waitpid(pid, &status, 0) < 0) {
                //manejo del caso si waitpid es interrumpido por una señal
                if (errno != EINTR) {
                    perror("waitpid");
                    break;
                }
            }
        }
    }

    return 0;
}