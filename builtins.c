#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "builtins.h"

int ejecutar_builtin(int argc, char *argv[], Job jobs[], int *num_jobs) {
    if (argc == 0 || argv[0] == NULL) {
        return 0;
    }

    //cd [dir]
    if (strcmp(argv[0], "cd")==0) {
        const char *destino = NULL;

        if (argc>1) {
            destino = argv[1];
        } else {
            //si no hay argumentos cambia hacia el directorio $HOME del usuario
            destino = getenv("HOME");
            if (destino == NULL) {
                fprintf(stderr, "cd: variable HOME no definida\n");
                return 1;
            }
        }

        //ejecuta la llamada a sistema chdir() para cambiar el directorio de la shell
        if (chdir(destino) != 0) {
            perror("cd");
        }
        //indica que se manejó como un comando interno
        return 1;
    }

    //exit [n]
    if (strcmp(argv[0], "exit")==0) {
        int codigo_salida = 0; //código por defecto: 0

        if (argc>1) {
            codigo_salida = atoi(argv[1]);
        }

        printf("Saliendo de miShell con estado %d...\n", codigo_salida);
        //termina el proceso principal de la shell
        exit(codigo_salida);
    }

    //jobs
    if (strcmp(argv[0], "jobs")==0) {
        if (*num_jobs == 0) {
            printf("No hay procesos en background.\n");
        } else {
            for (int i =0; i < *num_jobs; i++) {
                printf("[%d] PID: %d | Estado: %s | Comando: %s\n",
                       jobs[i].id,
                       jobs[i].pid,
                       jobs[i].estado,
                       jobs[i].comando);
            }
        }
        return 1;
    }

    //no es un comando interno, debe ejecutarse con fork()+execvp()
    return 0;
}