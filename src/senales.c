#include "senales.h"
#include <signal.h>
#include <stddef.h>

//Configura la shell para que ignore SIGINT (Ctrl+C) y SIGQUIT (Ctrl+\)
//Se usa para que el propio shell no muera al recibir estas señales,
//solo deben afectar el proceso en primer plano (foreground) que esta ejecutando.
void configurar_senales_shell(){
    struct sigaction sa;

    //Ignorar la señal
    sa.sa_handler=SIG_IGN;
    //No bloquear señales adicionales mientras se maneja esta
    sigemptyset(&sa.sa_mask);
    sa.sa_flags=0;

    //Ignorar (Ctrl+C)
    sigaction(SIGINT, &sa, NULL);
    //Ignorar (Ctrl+\)
    sigaction(SIGQUIT, &sa, NULL);
}

//Restaura el comportamiento por defecto de SIGINT y SIGQUIT
//Se usa en el proceso hijo (antes de execvp) para que el,
//proceso foreground si pueda ser interrumpido normalmente
//ya que heredaria el SIG_IGN del shell si no se restaura
void restaurar_senales_foreground(){
    struct sigaction sa;

    //Comportamiento por defecto del sistema
    sa.sa_handler=SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags=0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
}