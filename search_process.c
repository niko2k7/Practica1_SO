#include <unistd.h>
#include <ctype.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "definitions.h"

// Búsqueda
void search_movie(const char *csv_filename, const char *index_filename, const char *movie_name, char *output) {

    FILE *csv = fopen(csv_filename, "r");
    if (!csv) {
        perror("Error abriendo el archivo CSV.");
        return;
    }

    FILE *index = fopen(index_filename, "rb");
    if (!index) {
        perror("Error abriendo el archivo de índice.");
        fclose(csv);
        return;
    }

    // Copiar y convertir el nombre a minúsculas
    char lower_name[256];
    strncpy(lower_name, movie_name, sizeof(lower_name));
    lower_name[sizeof(lower_name) - 1] = '\0';

    for (int i = 0; lower_name[i]; i++) {
        lower_name[i] = tolower((unsigned char)lower_name[i]);
    }

    // Calcular hash
    int len = strlen(lower_name);
    if (len > CHAR_LENGTH) len = CHAR_LENGTH;

    uint32_t hash = MurmurHash2(lower_name, len, SEED);
    uint32_t index_pos = hash % TABLE_SIZE;

    // Buscar bucket
    IndexEntry entry;
    fseek(index, index_pos * sizeof(IndexEntry), SEEK_SET);
    fread(&entry, sizeof(IndexEntry), 1, index);

    // Seguir lista de colisiones
    while (1) {
        if (entry.hash == hash) {
            // Leer la línea correspondiente en el CSV
            char line[8192];
            fseek(csv, entry.offset, SEEK_SET);
            if (fgets(line, sizeof(line), csv)) {
                snprintf(output, RESULT_SIZE, "%s", line);  // Copia al área compartida
                printf("\nPelícula encontrada:\n%s\n", line);
                // strncat(output, line, RESULT_SIZE - strlen(output) - 1); //para imprimir cada coincidencia de hash
            } else {
                snprintf(output, RESULT_SIZE, "Error: No se pudo leer la línea en el CSV.\n");
                printf("No se pudo leer la línea en el CSV.\n");
            }
            fclose(csv);
            fclose(index);
            return;
        }

        if (entry.next == -1) break;

        fseek(index, entry.next * sizeof(IndexEntry), SEEK_SET);
        fread(&entry, sizeof(IndexEntry), 1, index);
    }

    printf("NA (Película No Encontrada)\n");

    fclose(csv);
    fclose(index);
}


int main(){
    key_t key = SHM_KEY_DATA;
    int shmid;
    SharedData *shared;

    // Conectar con el mismo segmento que el UI
    shmid = shmget(key, sizeof(SharedData), 0666);
    if (shmid < 0) {
        perror("Error al conectar memoria compartida");
        exit(1);
    }

    shared = (SharedData *)shmat(shmid, NULL, 0);
    if (shared == (void *)-1) {
        perror("Error al adjuntar memoria compartida");
        exit(1);
    }

    printf("Proceso de búsqueda en espera...\n");

    while (1) {
        if (shared->flag == READY) {
            search_movie(DATASET_FILENAME, INDEX_FILENAME, shared->movie_name, shared->result);
            shared->flag = NOT_READY;
        } else if (shared->flag == EXIT) {
            printf("Finalizando proceso de búsqueda.\n");
            break;
        }
        usleep(1000); // evitar consumo excesivo de CPU
    }

    shmdt(shared);
    return 0;
}
