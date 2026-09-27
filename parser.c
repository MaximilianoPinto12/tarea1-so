#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "parser.h"

void mostrar_prompt(void) {
    char cwd[PATH_MAX];
    
    //obtiene el directorio de trabajo actual usando getcwd
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("miShell:%s$ ", cwd);
    } else {
        perror("getcwd");
        printf("miShell:$ ");
    }
    //asegura que el prompt se imprima de inmediato antes de leer
    fflush(stdout); 
}

char *leer_linea(char *buffer, int size) {
    //fgets lee una línea completa desde stdin hasta un salto de línea (\n) o EOF (Ctrl+D)
    char *res = fgets(buffer, size, stdin);
    return res;
}

int parsear_linea(char *linea, char *argv[]) {
    int argc = 0;
    
    //separadores válidos: espacio, tabulación, retorno de carro, salto de línea
    const char *delimitadores = " \t\r\n";
    
    //toma el primer token de la cadena
    char *token = strtok(linea, delimitadores);
    
    //itera para tomar todos los argumentos respetando espacios múltiples
    while (token != NULL && argc < MAX_ARGS - 1) {
        argv[argc++] = token;
        token = strtok(NULL, delimitadores);
    }
    
    //ultimo elemento debe ser obligatoriamente NULL para funciones como execvp
    argv[argc] = NULL;
    
    return argc;
}