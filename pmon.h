#ifndef PMON_H
#define PMON_H

typedef struct{
    int pid;
    unsigned long tiempo_anterior;
    int activo;
}ProcesoBG;

void ejecutar_pmon(ProcesoBG lista_procesos[],int cantidad_procesos,int segundos);

#endif