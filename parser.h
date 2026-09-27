#ifndef PARSER_H
#define PARSER_H

#define MAX_LINE_LEN 1024
#define MAX_ARGS 64

void mostrar_prompt(void);

char *leer_linea(char *buffer, int size);

int parsear_linea(char *linea, char *argv[]);

#endif 