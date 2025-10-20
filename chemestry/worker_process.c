#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h> 
#include <unistd.h>
#include <time.h>
#include "definitions.h" 

// El puntero hash_table global 
IndexEntry *hash_table = NULL; 

// Busca la clave SMILES y escribe el resultado directamente en el buffer de la SHM.
void search_key(FILE *csv_file, FILE *index_file, SharedMessage *shm_ptr) {
    
    const char *key = shm_ptr->search_key1;
    
    // Calcular Hash y posición inicial en RAM
    int len = strlen(key);
    uint32_t hash = MurmurHash2(key, len, SEED);
    uint32_t index_pos = hash % TABLE_SIZE;

    IndexEntry current_entry = hash_table[index_pos];

    shm_ptr->response_buffer[0] = '\0'; 
    int found = 0;
    
    char *response_ptr = shm_ptr->response_buffer;
    size_t remaining_size = sizeof(shm_ptr->response_buffer);

    // Si el slot en RAM está vacío
    if (current_entry.hash == 0 && current_entry.offset == 0 && current_entry.next == -1) {
        snprintf(response_ptr, remaining_size, "Búsqueda finalizada.\nResultado: No se encontraron coincidencias para la clave SMILES '%s'.\n", key);
        return;
    }

    // Iteración principal 
    while (1) {
        if (current_entry.hash == hash) {
            
            if (fseek(csv_file, current_entry.offset, SEEK_SET) == 0) {
                
                char line[CSV_LINE_MAX];
                if (fgets(line, sizeof(line), csv_file) != NULL) {
                    
                    char temp_line[CSV_LINE_MAX];
                    strcpy(temp_line, line);
                    
                    strtok(temp_line, ",");
                    strtok(NULL, ",");
                    char *found_key = strtok(NULL, ","); 

                    if (found_key) {
                        
                        // Limpiar commillas
                        found_key[strcspn(found_key, "\r\n")] = 0;
                        while (found_key[0] == '"') { memmove(found_key, found_key + 1, strlen(found_key)); }
                        size_t len_found = strlen(found_key);
                        while (len_found > 0 && found_key[len_found - 1] == '"') { found_key[len_found - 1] = '\0'; len_found--; }
                        
                        if (strcmp(found_key, key) == 0) {
                            found = 1;

                            // Escribir la coincidencia directamente en la memoria compartida
                            snprintf(response_ptr, remaining_size, "Coincidencia encontrada:\n%s\n", line);
                            response_ptr += strlen(response_ptr);
                            remaining_size -= strlen(response_ptr);
                            
                            break; 
                        }
                    }
                }
            }
        }
        
        // Avanzar a la siguiente colisión
        int32_t next_index = current_entry.next;
        
        if (next_index != -1) {
            fseek(index_file, (long)next_index * sizeof(IndexEntry), SEEK_SET);
            if (fread(&current_entry, sizeof(IndexEntry), 1, index_file) != 1) {
                break;
            }
        } else {
            break; 
        }

    }
    
    // Imprimir final de la búsqueda
    if (!found) {
        snprintf(shm_ptr->response_buffer, sizeof(shm_ptr->response_buffer), "(NA) Búsqueda finalizada.\nResultado: No se encontraron coincidencias para la clave SMILES '%s'.\n", key);
    } else {
        // Si ya escribió la coincidencia, solo añade el final
        snprintf(shm_ptr->response_buffer + strlen(shm_ptr->response_buffer), sizeof(shm_ptr->response_buffer) - strlen(shm_ptr->response_buffer), "Búsqueda finalizada.\n");
    }
}


void run_worker(const char *csv_filename, const char *index_filename, SharedMessage *shm_ptr) {
    
    // Configuración de memoria compartida
    shm_ptr->worker_pid = getpid();
    
    // Cargar la tabla hash de disco a RAM
    size_t table_size_bytes = (size_t)TABLE_SIZE * sizeof(IndexEntry);
    hash_table = (IndexEntry *)malloc(table_size_bytes);
    if (!hash_table) { perror("#WORKER -> Error de malloc para la tabla hash"); exit(EXIT_FAILURE); }
    FILE *index_file_disk = fopen(index_filename, "rb");
    if (!index_file_disk) { perror("#WORKER -> Error abriendo hash_index.bin"); free(hash_table); exit(EXIT_FAILURE); }
    if (fread(hash_table, sizeof(IndexEntry), TABLE_SIZE, index_file_disk) != TABLE_SIZE) { fprintf(stderr, "#WORKER -> Error leyendo el índice.\n"); }
    printf("#WORKER -> Tabla hash cargada en RAM (%.3f MB). Esperando comandos...\n", (float)table_size_bytes / (1024 * 1024));
    
    // Abrir el CSV 
    FILE *csv_file = fopen(csv_filename, "r");
    if (!csv_file) { fclose(index_file_disk); free(hash_table); exit(EXIT_FAILURE); }
    
    // Bucle principal de escucha y manejo de memoria compartida
    while (1) {
        // Esperar hasta que la UI envíe un comando
        while (shm_ptr->command == CMD_NONE) {
            usleep(10000); // Esperar 10000μs -> 10ms (polling)
        }

        if (shm_ptr->command == CMD_SEARCH) {
            printf("#WORKER -> Buscando: '%s'...\n", shm_ptr->search_key1);
            
            // Ejecutar búsqueda y escribir respuesta en SHM 
            clock_t start = clock();
            search_key(csv_file, index_file_disk, shm_ptr); 
            clock_t end = clock();
            double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
            
            printf("#WORKER -> Búsqueda finalizada en %.7f segundos.\n", time_spent);
            
            // Señalizar a la UI que la búsqueda terminó
            shm_ptr->command = CMD_NONE; 

        } else if (shm_ptr->command == CMD_EXIT) {
            printf("#WORKER -> Comando EXIT recibido. Terminando...\n");
            break;
        }
    }

    // Solo se desmonta y elimina en main.c. Entonces solo liberamos la RAM interna.
    fclose(csv_file);
    fclose(index_file_disk);
    free(hash_table);
}
