// main.c (Coordinador de Procesos)

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "definitions.h"

// Prototipos de las funciones modulares (implementadas en otros .c)
void run_ui();
void run_worker(const char *csv_filename, const char *index_filename);

/**
 * @brief Elimina los archivos FIFO.
 */
void cleanup_fifos() {
    unlink(FIFO_UI_TO_WORKER);
    unlink(FIFO_WORKER_TO_UI);
}

int main() {
    pid_t pid;

    // 1. Crear las Tuberías Nombradas (FIFOs)
    cleanup_fifos();
    
    if (mkfifo(FIFO_UI_TO_WORKER, 0666) == -1 && errno != EEXIST) {
        perror("[MAIN] Error creando FIFO_UI_TO_WORKER");
        return 1;
    }
    if (mkfifo(FIFO_WORKER_TO_UI, 0666) == -1 && errno != EEXIST) {
        perror("[MAIN] Error creando FIFO_WORKER_TO_UI");
        cleanup_fifos();
        return 1;
    }
    printf("[MAIN] FIFOs creados.\n");


    // 2. Crear los procesos (fork)
    pid = fork();

    if (pid < 0) {
        perror("[MAIN] Error en fork");
        cleanup_fifos();
        return 1;
    } 
    
    // --- Proceso Hijo (Worker) ---
    if (pid == 0) {
        run_worker(DATASET_FILENAME, INDEX_FILENAME);
        exit(0);
    } 
    
    // --- Proceso Padre (UI) ---
    else {
        run_ui();
        
        // Espera a que el Worker (hijo) termine
        int status;
        waitpid(pid, &status, 0);
        
        // Limpia FIFOs
        cleanup_fifos();
        printf("[MAIN] Programa finalizado y recursos liberados.\n");
    }

    return 0;
}