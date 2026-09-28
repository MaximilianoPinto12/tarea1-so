#ifndef SENALES_H
#define SENALES_H

// Llama a esta función una sola vez al inicio del main() de la shell
void configurar_senales_shell();

// Llama a esta función dentro del hijo ANTES de execvp(), 
// solo si el comando es de primer plano (no tiene &).
void restaurar_senales_foreground();

#endif