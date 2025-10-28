#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include "definitions.h"

int main(){
    key_t key = SHM_KEY_DATA;
    int shmid;
    SharedData *shared;

    // Crear o conectar la memoria compartida
    shmid = shmget(key, sizeof(SharedData), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("Error al crear memoria compartida");
        exit(1);
    }

    shared = (SharedData *)shmat(shmid, NULL, 0);
    if (shared == (void *)-1) {
        perror("Error al conectar memoria compartida");
        exit(1);
    }

    // Inicializamos el flag
    shared->flag = NOT_READY;

    int opcion = 0;
    char movie_name[MOVIE_NAME_SIZE];

    while (opcion != 3) {
        printf("\n=== Menú ===\n");
        printf("1. Ingresar nombre de película\n");
        printf("2. Buscar\n");
        printf("3. Salir\n");
        printf("Seleccione opción: ");
        scanf("%d", &opcion);
        getchar(); // limpiar buffer

        switch (opcion) {
            case 1:
                printf("Ingrese el nombre de la película: ");
                fgets(movie_name, sizeof(movie_name), stdin);
                movie_name[strcspn(movie_name, "\r\n")] = 0;
                strncpy(shared->movie_name, movie_name, MOVIE_NAME_SIZE);
                printf("Película '%s' lista para buscar.\n", movie_name);
                break;

            case 2:

                struct timespec start, end;
                clock_gettime(CLOCK_MONOTONIC, &start); // inicio
                // memset(shared->result, 0, sizeof(shared->result));
                shared->flag = READY;
                printf("Esperando resultado...\n");

                // Esperar a que el proceso de búsqueda actualice el resultado
                while (shared->flag == READY) {
                    usleep(1);
                }

                printf("\n=== Resultado ===\n%s\n", shared->result);

                clock_gettime(CLOCK_MONOTONIC, &end); // fin
                double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

                printf("Tiempo total de búsqueda: %.6f segundos\n", elapsed);
                break;

            case 3:
                printf("Saliendo...\n");
                shared->flag = EXIT;
                break;

            default:
                printf("Opción no válida.\n");
        }
    }

    shmdt(shared);
    shmctl(shmid, IPC_RMID, NULL); // eliminar segmento

    return 0;
}