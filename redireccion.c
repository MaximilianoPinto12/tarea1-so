#include "redireccion.h"
#include "senales.h"
#include "jobs.h"
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

//Ejecuta un comando en un proceso hijo, con soporte opcional de
//redirección de entrada (archivo_entrada) y salida (archivo_salida).
//es_append indica si la salida se debe truncar (0) o agregar al final (1).
//en_background indica si el comando se lanza con & (1) o en foreground (0).
void ejecutar_con_redireccion(char **args, char *archivo_entrada, char *archivo_salida, int es_append, int en_background){
    pid_t pid=fork();

    if (pid<0){
        //fork() fallo y no se pudo crear el proceso hijo
        perror("Error en fork");
        return;
    }

    if (pid==0){
        //Codigo del proceso hijo

        //Solo el proceso en foreground debe poder morir con Ctrl+C / Ctrl+\
        //El de background conserva el SIG_IGN heredado de la shell
        if (!en_background){
            restaurar_senales_foreground();
        }

        //Redirección de entrada si se especifico un archivo
        if (archivo_entrada!=NULL){
            int fd_in=open(archivo_entrada, O_RDONLY);
            if (fd_in<0){
                perror("Error al abrir archivo de entrada");
                _exit(EXIT_FAILURE);
            }
            //STDIN pasa a leer de ese archivo
            if (dup2(fd_in, STDIN_FILENO)<0){
                perror("Error en dup2 para entrada");
                _exit(EXIT_FAILURE);
            }
            close(fd_in);
        }

        //Redirección de salida si se especifico un archivo
        if (archivo_salida!=NULL){
            int flags=O_WRONLY | O_CREAT;

            if (es_append){
                //Agregar al final (>>)
                flags |= O_APPEND;
            }else{
                //Sobrescribir (>)
                flags |= O_TRUNC;
            }
            int fd_out=open(archivo_salida, flags, 0644);
            if (fd_out<0){
                perror("Error al abrir archivo de salida");
                _exit(EXIT_FAILURE);
            }
            //STDOUT pasa a escribir en ese archivo
            if (dup2(fd_out, STDOUT_FILENO)<0){
                perror("Error en dup2 para salida");
                _exit(EXIT_FAILURE);
            }
            close(fd_out);
        }

        //Reemplaza la imagen del proceso hijo por el comando a ejecutar
        //Si execvp tiene exito, nunca regresa
        execvp(args[0], args);

        perror("Error en execvp");
        //_exit para no vaciar los buffers de stdio heredados del padre
        _exit(127);
    }else{
        //Codigo del proceso padre
        if (en_background){
            //No se espera al hijo: el handler de SIGCHLD lo recolectara
            agregar_job(pid, args[0]);
        }else{
            int status;
            //Espera que el hijo termine
            //Se reintenta si una señal interrumpe (EINTR). Si el handler de SIGCHLD
            //ya recolecto al hijo, waitpid falla con ECHILD y se sale sin problema
            while (waitpid(pid, &status, 0)<0){
                if (errno!=EINTR){
                    break;
                }
            }
        }
    }
}