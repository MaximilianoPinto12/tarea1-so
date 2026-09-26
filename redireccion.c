#include "redireccion.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

// Esta función simula el momento de la ejecución. 
// Recibe el comando limpio (args) y los archivos de redirección (si existen).
void ejecutar_comando(char **args, char *archivo_entrada, char *archivo_salida, int es_append) {
    pid_t pid = fork(); // Única forma permitida de crear el proceso

    if (pid < 0) {
        perror("Error en fork");
        return;
    } 
    
    if (pid == 0) {
        // --- ESTAMOS EN EL PROCESO HIJO ---

        // 1. Redirección de entrada (<)
        if (archivo_entrada != NULL) {
            // Se abre en modo de solo lectura
            int fd_in = open(archivo_entrada, O_RDONLY);
            if (fd_in < 0) {
                perror("Error al abrir archivo de entrada");
                exit(EXIT_FAILURE); // Termina el hijo si el archivo no existe
            }
            
            // Reemplaza stdin (0) por el archivo
            if (dup2(fd_in, STDIN_FILENO) < 0) {
                perror("Error en dup2 para entrada");
                exit(EXIT_FAILURE);
            }
            close(fd_in); // Se cierra el descriptor original para no dejar basura
        }

        // 2. Redirección de salida (> o >>)
        if (archivo_salida != NULL) {
            int flags = O_WRONLY | O_CREAT;
            
            if (es_append) {
                flags |= O_APPEND; // Modo >> (agrega al final)
            } else {
                flags |= O_TRUNC;  // Modo > (sobrescribe desde cero)
            }

            // 0644 asigna permisos de lectura/escritura (rw-r--r--)
            int fd_out = open(archivo_salida, flags, 0644);
            if (fd_out < 0) {
                perror("Error al abrir archivo de salida");
                exit(EXIT_FAILURE);
            }
            
            // Reemplaza stdout (1) por el archivo
            if (dup2(fd_out, STDOUT_FILENO) < 0) {
                perror("Error en dup2 para salida");
                exit(EXIT_FAILURE);
            }
            close(fd_out);
        }

        // 3. Ejecutar el comando
        // Como ya redirigimos los descriptores, execvp escribirá/leerá 
        // automáticamente de los archivos correctos.
        execvp(args[0], args);
        
        // Si execvp falla (ej. comando mal escrito), el código llega aquí.
        perror("Error en execvp");
        exit(EXIT_FAILURE); 
    } 
    else {
        // --- ESTAMOS EN EL PROCESO PADRE (SHELL) ---
        // Aquí iría la lógica de waitpid() o el manejo de procesos en background (&)
        int status;
        waitpid(pid, &status, 0); 
    }
}