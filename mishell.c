#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "builtins.h"
#include "jobs.h"
#include "senales.h"
#include "redireccion.h"
#include "pipes.h"

int main(void){
    char linea[MAX_LINE_LEN];
    char buf[TOKEN_BUF_SIZE];
    char *tokens[MAX_TOKENS];
    int quoted[MAX_TOKENS];
    Comando c;

    //La shell ignora SIGINT y SIGQUIT, y recolecta los hijos en background con SIGCHLD
    configurar_senales_shell();
    inicializar_manejador_sigchld();

    while (1){
        //Avisa de los jobs en background que terminaron desde el ultimo prompt
        notificar_jobs_terminados();
        mostrar_prompt();

        //NULL significa EOF (Ctrl+D): se termina la shell limpiamente
        if (leer_linea(linea, sizeof(linea))==NULL){
            printf("\n");
            break;
        }

        int n=parsear_linea(linea, tokens, quoted, buf, MAX_TOKENS);
        if (n==-1){
            fprintf(stderr, "miShell: error de sintaxis: comilla sin cerrar\n");
            continue;
        }
        if (n==-2){
            fprintf(stderr, "miShell: demasiados tokens (maximo %d)\n", MAX_TOKENS-1);
            continue;
        }
        //Linea vacia: se muestra el prompt de nuevo
        if (n==0){
            continue;
        }

        //Un '&' final (sin comillas) pide ejecucion en background
        int en_bg=(!quoted[n-1] && strcmp(tokens[n-1], "&")==0);
        if (en_bg){
            tokens[--n]=NULL;
            if (n==0){
                fprintf(stderr, "miShell: error de sintaxis: falta el comando\n");
                continue;
            }
        }

        if (construir_comandos(tokens, quoted, n, &c)<0){
            continue;
        }

        //Comandos internos: se ejecutan en la propia shell, sin fork
        if (c.num_comandos==1 && ejecutar_builtin(c.argcs[0], c.cmds[0])){
            continue;
        }

        if (c.num_comandos==1){
            ejecutar_con_redireccion(c.cmds[0], c.archivo_entrada, c.archivo_salida, c.es_append, en_bg);
        }else{
            //ejecutar_pipe no maneja archivos de redireccion, se rechaza en vez de ignorarlos
            if (c.archivo_entrada!=NULL || c.archivo_salida!=NULL){
                fprintf(stderr, "miShell: redirección dentro de tuberías no soportada\n");
                continue;
            }
            ejecutar_pipe(c.num_comandos, c.comandos, en_bg);
        }
    }
    return 0;
}