#include "pmon.h"
#include <unistd.h>
//main temporal
int main(){
    ProcesoBG mis_jobs[2];
    
    mis_jobs[0].pid=getpid();
    mis_jobs[0].tiempo_anterior=0;
    mis_jobs[0].activo=1;

    mis_jobs[1].pid=1;
    mis_jobs[1].tiempo_anterior=0;
    mis_jobs[1].activo=1;

    ejecutar_pmon(mis_jobs,2,2);
    return 0;
}