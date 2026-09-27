#include "pipes.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

//Ejecutar una cadena de comandos conectadas por pipes
//num_comandos indica cuantos comandos hay y comandos [i] es el argv de cada uno
void ejecutar_pipe(int num_comandos, char ***comandos){
    int p[2];
    //Descriptor de entrada para el comando actual (0=stdin real, el primero no redirige)
    int fd_in=0;
    pid_t pid;

    for (int i=0; i<num_comandos; i++){
        //Si no es el ultimo comando, se necesita un pipe para conectarlo con el siguiente
        if (i<num_comandos-1){
            if (pipe(p)<0){
                perror("Error al crear el pipe");
                exit(EXIT_FAILURE);
            }
        }

        pid=fork();

        if (pid<0){
            perror("Error en fork");
            exit(EXIT_FAILURE);
        }
        if (pid==0){
            //Codigo del proceso hijo
            restaurar_senales_foreground();
            //Si no es el primer comando, su entrada viene del pipe anterior
            if (fd_in !=0){
                if (dup2(fd_in, STDIN_FILENO)<0){
                    perror("Error dup2 fd_in");
                    exit(EXIT_FAILURE);
                }
                close(fd_in);
            }

            //Si no es el último comando, su salida va al pipe actual
            if (i<num_comandos-1){
                if (dup2(p[1], STDOUT_FILENO)<0){
                    perror("Error en dup2 p[1]");
                    exit(EXIT_FAILURE);
                }
                close(p[1]);
                close(p[0]);
            }

            //Ya no necesitan los extremos originales del pipe
            execvp(comandos[i][0], comandos[i]);
            perror("Error en execvp");
            exit(EXIT_FAILURE);
        }else{
            //Codigo del proceso padre
            //El padre ya no necesita el extremo de lectura anterior (el hijo ya lo heredo/duplico)
            if (fd_in !=0){
                close(fd_in);
            }

            if (i<num_comandos-1){
                //El padre no escribe en el pipe, asi que cierra el extremo de la escritura
                close(p[1]);
                //Guarda el extremo de lectura para que el siguiente comando lo use como entrada
                fd_in=p[0];
            }
        }
    }

    //Espera a que todos los procesos hijos terminen
    for (int i=0; i<num_comandos; i++){
        waitpid(-1, NULL, 0);
    }
}