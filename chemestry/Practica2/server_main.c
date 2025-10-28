#include <stdio.h>
#include <stdlib.h>
#include "definitions.h" 

// Declaración de la función Worker que se encuentra en worker_process.c
int run_worker(const char *csv_filename, const char *index_filename);

int main(int argc, char *argv[]) {
    
    printf("--- Iniciador del servidor Worker ---\n");
    printf("#SERVER_MAIN -> Cargando índice y esperando conexiones en el puerto %d...\n", SOCKET_PORT);
    
    if (run_worker(DATASET_FILENAME, INDEX_FILENAME) != 0) {
        fprintf(stderr, "#SERVER_MAIN -> El Worker terminó con errores.\n");
        return EXIT_FAILURE;
    }

    printf("#SERVER_MAIN -> Servidor Worker finalizado.\n");
    return EXIT_SUCCESS;
}