#include "pipes.h"
#include "senales.h"
#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

//Ejecutar una cadena de comandos conectadas por pipes
//num_comandos indica cuantos comandos hay y comandos [i] es el argv de cada uno
//en_background indica si toda la tuberia se lanza con & (1) o en foreground (0)
void ejecutar_pipe(int num_comandos, char ***comandos, int en_background){
    int p[2];
    //Descriptor de entrada para el comando actual (0=stdin real, el primero no redirige)
    int fd_in=0;
    //PID de cada hijo, para esperar a cada uno por separado (sin waitpid(-1))
    //asi no se le roba a la shell un hijo en background que le corresponde al handler de SIGCHLD
    pid_t pids[num_comandos];
    int lanzados=0;

    for (int i=0; i<num_comandos; i++){
        //Si no es el ultimo comando, se necesita un pipe para conectarlo con el siguiente
        if (i<num_comandos-1){
            if (pipe(p)<0){
                perror("Error al crear el pipe");
                break;
            }
        }

        pid_t pid=fork();

        if (pid<0){
            perror("Error en fork");
            //Se cierra el pipe recien creado para no dejar descriptores abiertos
            if (i<num_comandos-1){
                close(p[0]);
                close(p[1]);
            }
            break;
        }
        if (pid==0){
            //Codigo del proceso hijo

            //Solo los procesos en foreground restauran las señales
            if (!en_background){
                restaurar_senales_foreground();
            }
            //Si no es el primer comando, su entrada viene del pipe anterior
            if (fd_in !=0){
                if (dup2(fd_in, STDIN_FILENO)<0){
                    perror("Error dup2 fd_in");
                    _exit(EXIT_FAILURE);
                }
                close(fd_in);
            }

            //Si no es el último comando, su salida va al pipe actual
            if (i<num_comandos-1){
                if (dup2(p[1], STDOUT_FILENO)<0){
                    perror("Error en dup2 p[1]");
                    _exit(EXIT_FAILURE);
                }
                close(p[1]);
                close(p[0]);
            }

            //Ya no necesitan los extremos originales del pipe
            execvp(comandos[i][0], comandos[i]);
            perror("Error en execvp");
            _exit(127);
        }else{
            //Codigo del proceso padre
            pids[lanzados++]=pid;

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

    //Si el ciclo termino antes de tiempo por un error, queda un extremo de lectura abierto
    if (fd_in !=0){
        close(fd_in);
    }

    if (en_background){
        //Se registra un solo job para toda la tuberia, con el PID del ultimo comando
        if (lanzados>0){
            agregar_job(pids[lanzados-1], comandos[0][0]);
        }
    }else{
        //Espera a que todos los procesos hijos terminen
        //Se reintenta si una señal interrumpe (EINTR). Si el handler de SIGCHLD
        //ya recolecto al hijo, waitpid falla con ECHILD y se sigue con el siguiente
        for (int i=0; i<lanzados; i++){
            while (waitpid(pids[i], NULL, 0)<0){
                if (errno!=EINTR){
                    break;
                }
            }
        }
    }
}