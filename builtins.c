#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "builtins.h"
#include "pmon.h"
 
int ejecutar_builtin(int argc, char *argv[]) {
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
        //lista_jobs tiene huecos (jobs ya terminados), asi que se recorre
        //completa y solo se muestran las casillas activas
        int hay_jobs = 0;
        for (int i = 0; i < MAX_JOBS; i++) {
            if (lista_jobs[i].activo) {
                printf("[%d] PID: %d | Estado: Ejecutando | Comando: %s\n",
                       lista_jobs[i].id,
                       (int)lista_jobs[i].pid,
                       lista_jobs[i].comando);
                hay_jobs = 1;
            }
        }
        if (!hay_jobs) {
            printf("No hay procesos en background.\n");
        }
        return 1;
    }
 
    //pmon 
    if (strcmp(argv[0], "pmon")==0) {
        //valor por defecto
        int segundos = 2;

        if (argc > 1) {
            segundos = atoi(argv[1]);
        }

        //llama a tu motor pasándole el arreglo de background de la shell
        ejecutar_pmon(lista_jobs, MAX_JOBS, segundos);
        
        //indica que la shell ya lo manejó
        return 1; 
    }

    //no es un comando interno, debe ejecutarse con fork()+execvp()
    return 0;
}