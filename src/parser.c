#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
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
    char *res;
 
    //fgets lee una línea completa desde stdin hasta un salto de línea (\n) o EOF (Ctrl+D)
    //si una señal interrumpe la lectura (EINTR) se reintenta en vez de tratarlo como EOF
    while ((res = fgets(buffer, size, stdin)) == NULL) {
        if (ferror(stdin) && errno == EINTR) {
            clearerr(stdin);
            continue;
        }
        return NULL; //EOF real
    }
 
    //si la linea llenó el buffer sin '\n', es demasiado larga: se descarta el resto
    //(si no, el pedazo sobrante se leería como si fuera otro comando)
    size_t len = strlen(res);
    if (len == (size_t)size - 1 && res[len - 1] != '\n') {
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
        fprintf(stderr, "miShell: línea demasiado larga (máximo %d caracteres)\n", size - 1);
        res[0] = '\0';
    }
    return res;
}

static int es_operador_char(char c) {
    return c == '|' || c == '<' || c == '>' || c == '&';
}
 
static int es_operador(const char *t) {
    return strcmp(t, "|") == 0 || strcmp(t, "<") == 0 ||
           strcmp(t, ">") == 0 || strcmp(t, ">>") == 0 ||
           strcmp(t, "&") == 0;
}

//expansion de variables de entorno: al encontrar '$' seguido de un nombre
//valido (letras, digitos, '_', sin empezar con digito), lo reemplaza por el
//valor de getenv(). Si la variable no existe, se reemplaza por nada (cadena
//vacia), igual que en bash. Un '$' que no es seguido de un nombre valido
//(por ejemplo "precio: $5" o un "$" solo al final) se copia tal cual.
//*pp queda apuntando despues del nombre consumido; *outp, despues de lo escrito.
static void expandir_variable(const char **pp, char **outp) {
    const char *p = *pp;
    char *out = *outp;
 
    p++; //salta el '$'
    if (isalpha((unsigned char)*p) || *p == '_') {
        char nombre[128];
        int len = 0;
        while ((isalnum((unsigned char)*p) || *p == '_') && len < (int)sizeof(nombre) - 1) {
            nombre[len++] = *p++;
        }
        nombre[len] = '\0';
 
        const char *valor = getenv(nombre);
        if (valor != NULL) {
            while (*valor) *out++ = *valor++;
        }
        //si valor es NULL (variable no definida), no se escribe nada
    } else {
        //"$" no seguido de un nombre valido: se deja el caracter literal
        *out++ = '$';
    }
 
    *pp = p;
    *outp = out;
}

int parsear_linea(const char *linea, char *tokens[], int quoted[], char *buf, int max_tokens) {
    int n = 0;
    char *out = buf;
    const char *p = linea;
 
    while (*p) {
        //salta espacios y tabulaciones (multiples)
        while (isspace((unsigned char)*p)) p++;
        if (*p == '\0') break;
 
        //deja espacio para el NULL final del arreglo
        if (n >= max_tokens - 1) return -2;
        tokens[n] = out;
        quoted[n] = 0;
 
        if (*p == '|' || *p == '<' || *p == '&') {
            //operador de un caracter, es un token propio aunque no haya espacios
            *out++ = *p++;
        } else if (*p == '>') {
            *out++ = *p++;
            if (*p == '>') *out++ = *p++; //">>"
        } else {
            //palabra normal: termina en espacio u operador
            while (*p && !isspace((unsigned char)*p) && !es_operador_char(*p)) {
                if (*p == '"' || *p == '\'') {
                    char q = *p++;  //recuerda cual comilla abrio
                    quoted[n] = 1;
                    while (*p && *p != q) {
                        //igual que en bash: $VAR se expande dentro de comillas
                        //dobles, pero NO dentro de comillas simples
                        if (q == '"' && *p == '$') {
                            expandir_variable(&p, &out);
                        } else {
                            *out++ = *p++;
                        }
                    }
                    if (*p == '\0') return -1; //comilla sin cerrar
                    p++;   //salta la comilla de cierre
                } else if (*p == '$') {
                    expandir_variable(&p, &out);
                } else {
                    *out++ = *p++;
                }
            }
        }
        *out++ = '\0';
        n++;
    }
    tokens[n] = NULL;
    return n;
}

int construir_comandos(char *tokens[], const int quoted[], int n, Comando *c) {
    memset(c, 0, sizeof(*c));
    int cmd = 0;  //indice del comando que se esta armando
    int argc = 0;  //cantidad de argumentos del comando actual
 
    for (int i = 0; i < n; i++) {
        char *t = tokens[i];
        int es_op = !quoted[i] && es_operador(t);
 
        if (!es_op) {
            if (argc >= MAX_ARGS - 1) {
                fprintf(stderr, "miShell: demasiados argumentos (máximo %d)\n", MAX_ARGS - 1);
                return -1;
            }
            c->cmds[cmd][argc++] = t;
            continue;
        }
 
        if (strcmp(t, "|") == 0) {
            if (argc == 0) {
                fprintf(stderr, "miShell: error de sintaxis: falta un comando antes de '|'\n");
                return -1;
            }
            c->cmds[cmd][argc] = NULL;
            c->argcs[cmd] = argc;
            if (++cmd >= MAX_CMDS) {
                fprintf(stderr, "miShell: demasiados comandos en la tubería (máximo %d)\n", MAX_CMDS);
                return -1;
            }
            argc = 0;
            continue;
        }
 
        if (strcmp(t, "&") == 0) {
            //el "&" final ya lo quito es_background(), uno en medio es un error
            fprintf(stderr, "miShell: '&' solo se permite al final de la línea\n");
            return -1;
        }
 
        //aqui t es <, > o >>: el siguiente token debe ser el nombre del archivo
        if (i + 1 >= n || (!quoted[i + 1] && es_operador(tokens[i + 1]))) {
            fprintf(stderr, "miShell: error de sintaxis: falta el archivo después de '%s'\n", t);
            return -1;
        }
        char *archivo = tokens[++i];
        if (strcmp(t, "<") == 0) {
            c->archivo_entrada = archivo;
        } else {
            c->archivo_salida = archivo;
            c->es_append = (strcmp(t, ">>") == 0);
        }
    }
 
    if (argc == 0) {
        if (cmd > 0)
            fprintf(stderr, "miShell: error de sintaxis: falta un comando después de '|'\n");
        else
            fprintf(stderr, "miShell: error de sintaxis: falta el comando\n");
        return -1;
    }
    c->cmds[cmd][argc] = NULL;
    c->argcs[cmd] = argc;
    c->num_comandos = cmd + 1;
    for (int i = 0; i < c->num_comandos; i++) {
        c->comandos[i] = c->cmds[i];
    }
    return 0;
}