#include "pmon.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

volatile sig_atomic_t bandera_refresco=1;
volatile sig_atomic_t bandera_pmon_activo=1;

typedef struct{
    int pid;
    char comando[256];
    const char* estado_str;
    double porcentaje_cpu;
    long rss;
}RegistroProceso;

void manejador_alarma(int signum){
    (void)signum;
    bandera_refresco=1;
}

void manejador_sigint_pmon(int signum){
    (void)signum;
    bandera_pmon_activo=0;
}

void pid_a_texto(int pid,char *texto){
    if(pid==0){
        strcpy(texto,"0");
        return;
    }

    int temporal=pid;
    int digitos=0;

    while(temporal>0){
        digitos++;
        temporal=temporal/10;
    }

    texto[digitos]='\0';

    for(int i=digitos-1;i>=0;i--){
        texto[i]=(pid%10)+'0';
        pid=pid/10;
    }
}

const char* traducir_estado(char letra){
    switch(letra){
        case 'R': return "ejecutando";
        case 'S': return "durmiendo";
        case 'Z': return "zombie";
        case 'T': return "detenido";
        case 'X': return "terminado";
        default: return "desconocido";
    }
}

void leer_datos_proceso(int pid, char *comando, char *estado, unsigned long *utime, unsigned long *stime){
    *estado='X';
    char ruta[256];
    char pid_texto[32];

    pid_a_texto(pid,pid_texto);
    strcpy(ruta,"/proc/");
    strcat(ruta,pid_texto);
    strcat(ruta,"/stat");

    FILE *archivo=fopen(ruta,"r");
    if(archivo==NULL){
        *estado='X';
        return;
    }

    if(fscanf(archivo, "%*d (%[^)]) %c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu", comando, estado, utime, stime)!=4){
        *estado='X';
    }
    fclose(archivo);
}

long leer_rss(int pid){
    char ruta[256];
    char pid_texto[32];

    pid_a_texto(pid,pid_texto);
    strcpy(ruta, "/proc/");
    strcat(ruta, pid_texto);
    strcat(ruta, "/status");

    FILE *archivo=fopen(ruta,"r");
    if(archivo==NULL){
        return 0;
    }
    char linea[256];
    long rss=0;

    while(fgets(linea,sizeof(linea),archivo)){
        if(strncmp(linea,"VmRSS:",6)==0){
            sscanf(linea, "VmRSS: %ld",&rss);
            break;
        }
    }
    fclose(archivo);
    return rss;
}

void ejecutar_pmon(Job lista_procesos[],int cantidad_procesos,int segundos){
    if(cantidad_procesos<=0){
        printf("no hay procesos en background");
        return;
    }
    if(segundos<=0){
        segundos=2;
    }

    struct sigaction sa_alrm;
    sa_alrm.sa_handler=manejador_alarma;
    sigemptyset(&sa_alrm.sa_mask);
    sa_alrm.sa_flags=0;
    sigaction(SIGALRM,&sa_alrm,NULL);

    struct sigaction sa_int, sa_int_viejo;
    sa_int.sa_handler=manejador_sigint_pmon;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags=0;
    sigaction(SIGINT, &sa_int, &sa_int_viejo);
    
    bandera_pmon_activo=1;
    bandera_refresco=1;

    while(bandera_pmon_activo){
        if(bandera_refresco){
            bandera_refresco=0;
            printf("\033c");
            printf("%-7s | %-15s | %-11s | %-12s | %-10s\n", "PID", "COMANDO", "ESTADO", "%CPU (aprox)", "RSS (KB)");

            RegistroProceso capturas[cantidad_procesos];
            int cantidad_activos=0;

            for(int i=0;i<cantidad_procesos;i++){
                if (lista_procesos[i].activo==0){
                    continue;
                }
                char comando[256];
                char estado;
                unsigned long utime=0, stime=0;

                int pid_actual=lista_procesos[i].pid;
                leer_datos_proceso(pid_actual,comando,&estado,&utime,&stime);

                if(estado=='X'){
                    lista_procesos[i].activo=0;
                    continue;
                }

                long rss=leer_rss(pid_actual);
                unsigned long tiempo_actual=utime+stime;
                double porcentaje_cpu=0.0;

                if(lista_procesos[i].tiempo_anterior!=0){
                    unsigned long delta_ticks=tiempo_actual-lista_procesos[i].tiempo_anterior;
                    long hz=sysconf(_SC_CLK_TCK);
                    double delta_segundos=(double)delta_ticks/hz;
                    porcentaje_cpu=(delta_segundos/segundos)*100.0;
                }
                lista_procesos[i].tiempo_anterior=tiempo_actual;
                capturas[cantidad_activos].pid=pid_actual;
                strcpy(capturas[cantidad_activos].comando,comando);
                capturas[cantidad_activos].estado_str=traducir_estado(estado);
                capturas[cantidad_activos].porcentaje_cpu=porcentaje_cpu;
                capturas[cantidad_activos].rss=rss;
                cantidad_activos++;
            }

            for(int i=0;i<cantidad_activos-1;i++){
                for(int j=0;j<cantidad_activos-i-1;j++){
                    if(capturas[j].porcentaje_cpu<capturas[j+1].porcentaje_cpu){
                        RegistroProceso temp=capturas[j];
                        capturas[j]=capturas[j+1];
                        capturas[j+1]=temp;
                    }
                }
            }

            for(int i=0;i<cantidad_activos;i++){
                if(i==0 && cantidad_activos>0){
                    printf("\033[1;32m%-7d | %-15s | %-11s | %-12.1f | %-10ld\033[0m\n", capturas[i].pid, capturas[i].comando, capturas[i].estado_str, capturas[i].porcentaje_cpu, capturas[i].rss);
                }
                else{
                    printf("%-7d | %-15s | %-11s | %-12.1f | %-10ld\n", capturas[i].pid, capturas[i].comando, capturas[i].estado_str, capturas[i].porcentaje_cpu, capturas[i].rss);
                }
            }

            fflush(stdout);
            alarm(segundos);
        }
        pause();
    }
    alarm(0);
    sigaction(SIGINT,&sa_int_viejo,NULL);
}