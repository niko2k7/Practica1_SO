#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h> 
#include <signal.h> 
#include "definitions.h"

// Definición del puntero a la SHM
SharedMessage *shm_msg = NULL;
int shm_msg_id = -1;

void run_worker(const char *csv_filename, const char *index_filename, SharedMessage *shm_ptr);
void run_ui(SharedMessage *shm_ptr);

// Función de limpieza de recursos
void cleanup(int signum) {
    if (shm_msg_id != -1) {
        printf("#MAIN -> Desmontando y eliminando memoria compartida de mensajes...\n");
        shmdt(shm_msg);
        shmctl(shm_msg_id, IPC_RMID, NULL);
    }
    if (signum != 0) {
        exit(0);
    }
}

int main() {
    
    // Configurar la memoria compartida 
    shm_msg_id = shmget(SHM_KEY_DATA, sizeof(SharedMessage), 0666 | IPC_CREAT);
    if (shm_msg_id < 0) {
        perror("#MAIN -> Error al crear memoria compartida");
        exit(EXIT_FAILURE);
    }

    shm_msg = (SharedMessage *)shmat(shm_msg_id, 0, 0);
    if (shm_msg == (SharedMessage *)-1) {
        perror("[MAIN] Error al adjuntar Memoria Compartida");
        shmctl(shm_msg_id, IPC_RMID, NULL); 
        exit(EXIT_FAILURE);
    }

    // Inicializar la memoria y establecer el manejador de señales
    shm_msg->command = CMD_NONE;
    shm_msg->worker_pid = 0;
    shm_msg->ui_pid = 0;
    signal(SIGINT, cleanup);

    printf("#MAIN -> Memoria Compartida creada y adjunta.\n");

    // Creación de procesos hijos
    pid_t pid_worker = fork();

    if (pid_worker == 0) {
        run_worker(DATASET_FILENAME, INDEX_FILENAME, shm_msg);
        exit(0);
    } else if (pid_worker < 0) {
        perror("#MAIN -> Error fork Worker");
        cleanup(0);
        exit(EXIT_FAILURE);
    }

    pid_t pid_ui = fork();

    if (pid_ui == 0) {
        sleep(1); 
        run_ui(shm_msg);
        exit(0);
    } else if (pid_ui < 0) {
        perror("#MAIN -> Error fork UI");
        if (pid_worker > 0) kill(pid_worker, SIGTERM);
        cleanup(0);
        exit(EXIT_FAILURE);
    }

    // Proceso padre (main) espera a que los hijos terminen
    int status;
    wait(NULL);
    wait(NULL);

    printf("#MAIN -> Programa finalizado y recursos liberados.\n");
    cleanup(0); 

    return 0;
}