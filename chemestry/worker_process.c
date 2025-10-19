 // worker_process.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <errno.h> // Necesario para manejar EINTR
#include "definitions.h" 

// Puntero global para la tabla hash cargada en RAM (única variable global permitida)
IndexEntry *hash_table = NULL; 

/**
 * @brief Función que busca la clave SMILES en el CSV usando la tabla hash en RAM.
 * @param fd_write Descriptor del FIFO de salida (Worker -> UI) para reportar resultados.
 */
void search_key(const char *key, FILE *csv_file, FILE *index_file, int fd_write) {
    
    // 1. Calcular Hash y posición inicial en RAM
    int len = strlen(key);
    uint32_t hash = MurmurHash2(key, len, SEED);
    uint32_t index_pos = hash % TABLE_SIZE;

    IndexEntry current_entry = hash_table[index_pos];

    char response_buffer[CSV_LINE_MAX * 2];
    int found = 0;
    
    // Si el slot en RAM está vacío, la clave no existe
    if (current_entry.hash == 0 && current_entry.offset == 0 && current_entry.next == -1) {
        snprintf(response_buffer, sizeof(response_buffer), "Búsqueda finalizada.\nResultado: No se encontraron coincidencias para la clave SMILES '%s'.\n", key);
        write(fd_write, response_buffer, strlen(response_buffer) + 1);
        return;
    }

    // 2. Iteración principal
    while (1) {
        if (current_entry.hash == hash) {
            
            if (fseek(csv_file, current_entry.offset, SEEK_SET) == 0) {
                
                char line[CSV_LINE_MAX];
                if (fgets(line, sizeof(line), csv_file) != NULL) {
                    
                    // C. Verificar la clave SMILES exacta (prevención de colisiones)
                    char temp_line[CSV_LINE_MAX];
                    strcpy(temp_line, line);
                    
                    strtok(temp_line, ",");
                    strtok(NULL, ",");
                    char *found_key = strtok(NULL, ","); 

                    if (found_key) {
                        
                        // --- LÓGICA DE LIMPIEZA DE COMILLAS CORREGIDA ---
                        found_key[strcspn(found_key, "\r\n")] = 0;
                        
                        // Limpieza robusta de comillas iniciales
                        while (found_key[0] == '"') {
                            memmove(found_key, found_key + 1, strlen(found_key));
                        }

                        // Limpieza robusta de comillas finales
                        size_t len_found = strlen(found_key);
                        while (len_found > 0 && found_key[len_found - 1] == '"') {
                            found_key[len_found - 1] = '\0';
                            len_found--;
                        }
                        // --- FIN LÓGICA DE LIMPIEZA ---

                        if (strcmp(found_key, key) == 0) {
                            found = 1;
                            snprintf(response_buffer, sizeof(response_buffer), "COINCIDENCIA ENCONTRADA:\n%s\n", line);
                            write(fd_write, response_buffer, strlen(response_buffer) + 1);
                            
                            break; 
                        }
                    }
                }
            }
        }
        
        // D. Avanzar a la siguiente colisión
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
    
    // 3. Reportar final de la búsqueda
    if (!found) {
        snprintf(response_buffer, sizeof(response_buffer), "Búsqueda finalizada.\nResultado: No se encontraron coincidencias para la clave SMILES '%s'.\n", key);
        write(fd_write, response_buffer, strlen(response_buffer) + 1);
    } else {
        snprintf(response_buffer, sizeof(response_buffer), "Búsqueda finalizada.\n");
        write(fd_write, response_buffer, strlen(response_buffer) + 1);
    }
}

/**
 * @brief Bucle principal del Worker. Inicializa recursos y escucha comandos.
 */
void run_worker(const char *csv_filename, const char *index_filename) {
    
    // 1. Cargar la tabla hash de disco a RAM
    size_t table_size_bytes = (size_t)TABLE_SIZE * sizeof(IndexEntry);
    hash_table = (IndexEntry *)malloc(table_size_bytes);
    if (!hash_table) {
        perror("[WORKER] Error de malloc para la tabla hash");
        exit(EXIT_FAILURE);
    }
    
    FILE *index_file_disk = fopen(index_filename, "rb");
    if (!index_file_disk) {
        perror("[WORKER] Error abriendo hash_index.bin. ¡Asegúrese de ejecutar index_builder primero!");
        free(hash_table);
        exit(EXIT_FAILURE);
    }
    
    if (fread(hash_table, sizeof(IndexEntry), TABLE_SIZE, index_file_disk) != TABLE_SIZE) {
        fprintf(stderr, "[WORKER] Error leyendo el índice. El archivo podría estar incompleto.\n");
    }
    printf("[WORKER] Tabla hash cargada en RAM (%.2f MB). Esperando comandos...\n", (float)table_size_bytes / (1024 * 1024));
    
    // 2. Abrir el CSV 
    FILE *csv_file = fopen(csv_filename, "r");
    if (!csv_file) {
        perror("[WORKER] Error abriendo el dataset CSV");
        fclose(index_file_disk);
        free(hash_table);
        exit(EXIT_FAILURE);
    }
    
    // 3. Bucle principal de escucha y manejo de FIFOs
    PipeMessage message;
    while (1) {
        
        // A. Abrir FD_READ (Bloqueante, espera a que la UI escriba)
        int fd_read = open(FIFO_UI_TO_WORKER, O_RDONLY);
        if (fd_read == -1) { perror("[WORKER] Error abriendo FIFO de lectura. Reintentando..."); sleep(1); continue; }

        // B. Leer mensaje (Bloqueante)
        ssize_t bytes_read = read(fd_read, &message, sizeof(PipeMessage));
        close(fd_read); // CERRAMOS: Permite que la UI lo abra en el siguiente ciclo

        if (bytes_read > 0) {
            
            if (message.command == CMD_SEARCH) {
                printf("[WORKER] Buscando: '%s'...\n", message.search_key1);
                
                // C. Abrir FD_WRITE
                int fd_write = open(FIFO_WORKER_TO_UI, O_WRONLY);
                if (fd_write == -1) { perror("[WORKER] Error abriendo FIFO de escritura."); continue; }
                
                // D. Ejecutar búsqueda y Escribir respuesta
                clock_t start = clock();
                search_key(message.search_key1, csv_file, index_file_disk, fd_write); 
                clock_t end = clock();
                double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
                
                close(fd_write); // CERRAMOS: CRÍTICO. Esto desbloquea el read() de la UI
                
                printf("[WORKER] Búsqueda finalizada en %.4f segundos.\n", time_spent);

            } else if (message.command == CMD_EXIT) {
                printf("[WORKER] Comando EXIT recibido. Terminando...\n");
                break;
            }
        } else if (bytes_read == 0) {
             printf("[WORKER] FIFO de lectura cerrado (UI terminó). Terminando...\n");
             break;
        } else if (bytes_read < 0 && errno != EINTR) {
             perror("[WORKER] Error de lectura en FIFO");
             break;
        }
    }

    // 5. Cleanup
    fclose(csv_file);
    fclose(index_file_disk);
    free(hash_table);
}