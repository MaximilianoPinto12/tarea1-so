# miShell

Shell de texto simplificada para Linux, desarrollada en C como Tarea 1 del curso
Sistemas Operativos, 2026. Implementa el ciclo leer-parsear-ejecutar, comandos internos,
redirección de entrada/salida, tuberías de largo arbitrario, ejecución en background y
manejo de señales, además de un monitor de procesos propio (`pmon`) basado en `/proc`.

## Requisitos

- Linux (usa `/proc` para `pmon`)
- `gcc` con soporte para `-std=gnu11`
- `make`

## Estructura del repositorio

```
tarea1-so/
├── README.md
└── src/
    ├── Makefile
    ├── mishell.c
    ├── parser.c / parser.h
    ├── builtins.c / builtins.h
    ├── redireccion.c / redireccion.h
    ├── pipes.c / pipes.h
    ├── senales.c / senales.h
    ├── jobs.c / jobs.h
    └── pmon.c / pmon.h
```

Todo el código fuente y el `Makefile` están dentro de `src/`.

## Compilación

```bash
cd src
make
```

Esto genera el ejecutable `mishell` dentro de `src/`, compilando todos los archivos
`.c` con:

```
gcc -Wall -Wextra -std=gnu11 -o mishell *.c
```

El proyecto compila sin advertencias.

Para limpiar el binario generado:

```bash
make clean
```

## Ejecución

```bash
./mishell
```

(desde dentro de `src/`, luego de compilar; o `./src/mishell` desde la raíz del repositorio).

La shell muestra un prompt con el directorio de trabajo actual:

```
miShell:/home/usuario$
```

Para salir: escribir `exit [código]`, o presionar `Ctrl+D` (EOF).

## Funcionalidad soportada

### Comandos internos

- `cd [dir]`: cambia el directorio de trabajo. Sin argumentos, usa `$HOME`.
- `exit [n]`: termina la shell con código de salida `n` (por defecto 0).
- `jobs`: lista los procesos en background lanzados por la shell.
- `pmon [segundos]`: monitorea en tiempo real los procesos en background (ver más abajo).

### Redirección de entrada/salida

```bash
comando < archivo_entrada
comando > archivo_salida
comando >> archivo_salida
sort < datos.txt > datos_ordenados.txt
```

### Tuberías (pipes) de largo arbitrario

```bash
cmd1 | cmd2 | cmd3 | ... | cmdN
```

> **Limitación conocida:** no se soporta combinar redirección de archivo con tuberías
> (por ejemplo `sort < in.txt | uniq > out.txt`). La shell detecta el caso y muestra un
> mensaje de error en vez de ejecutar el comando parcialmente.

### Ejecución en background

```bash
sleep 30 &
```

La shell muestra de inmediato `[id] PID` y recolecta el término del proceso de forma
asíncrona (vía `SIGCHLD`), avisando en el prompt siguiente:

```
[1]+ Done    sleep 30
```

### Señales

- La shell ignora `SIGINT` (Ctrl+C) y `SIGQUIT` (Ctrl+\\): no se cierra al presionarlas.
- Un comando en foreground sí puede ser interrumpido/terminado con Ctrl+C o Ctrl+\\.
- Los procesos en background no se ven afectados por Ctrl+C enviado al terminal.

### `pmon`: monitor de procesos

```bash
pmon [segundos]
```

Muestra una tabla que se refresca cada `segundos` (por defecto 2) con los procesos en
background activos, leyendo directamente `/proc/[pid]/stat` y `/proc/[pid]/status`:

```
PID     | COMANDO         | ESTADO      | %CPU (aprox) | RSS (KB)
4821    | sleep           | durmiendo   | 0.0          | 412
4830    | yes             | ejecutando  | 97.3         | 360
```

Se sale de `pmon` con `Ctrl+C`, volviendo al prompt normal sin cerrar la shell.

## Estructura del código (`src/`)

| Archivo | Responsabilidad |
|---|---|
| `mishell.c` | `main`: ciclo leer-parsear-ejecutar |
| `parser.c` / `parser.h` | Prompt, lectura de línea, tokenización y armado de comandos |
| `builtins.c` / `builtins.h` | Comandos internos (`cd`, `exit`, `jobs`, `pmon`) |
| `redireccion.c` / `redireccion.h` | Ejecución de un comando simple con redirección de E/S |
| `pipes.c` / `pipes.h` | Ejecución de tuberías de N comandos |
| `senales.c` / `senales.h` | Manejo de `SIGINT`/`SIGQUIT` (shell vs. foreground) |
| `jobs.c` / `jobs.h` | Lista de jobs en background y manejador de `SIGCHLD` |
| `pmon.c` / `pmon.h` | Monitor de procesos vía `/proc`, con `SIGALRM` |

> **Nota:** la shell expande `$VAR` usando variables del entorno del sistema
> (`getenv`). No cuenta con un mecanismo para definir variables propias.

## Autores
- Martin Ignacio Carrasco Perez.
- Maximiliano Enrique Pinto Valenzuela.
- Alonso Ignacio Vergara Olivari.

Tarea 1 de Sistemas Operativos 2026.