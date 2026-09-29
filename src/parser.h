#ifndef PARSER_H
#define PARSER_H
 
#define MAX_LINE_LEN 1024
#define MAX_ARGS 64
#define MAX_CMDS 32 
#define MAX_TOKENS 256
#define TOKEN_BUF_SIZE (2*MAX_LINE_LEN + 2) 
 
typedef struct {
    int num_comandos; //1 = comando simple, >1 = tuberia
    char *cmds[MAX_CMDS][MAX_ARGS]; 
    char **comandos[MAX_CMDS]; //cmds[i] como arreglo de punteros: es lo que recibe ejecutar_pipe()
    int argcs[MAX_CMDS]; //cantidad de argumentos de cada comando
    char *archivo_entrada; //NULL si no hay '<'
    char *archivo_salida; //NULL si no hay '>' ni '>>'
    int es_append; //1 si fue '>>'
} Comando;
 
void mostrar_prompt(void);
 
char *leer_linea(char *buffer, int size);
 
int parsear_linea(const char *linea, char *tokens[], int quoted[], char *buf, int max_tokens);
 
int construir_comandos(char *tokens[], const int quoted[], int n, Comando *c);
 
#endif