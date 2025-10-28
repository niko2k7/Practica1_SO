#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "definitions.h" 

// Variables Globales de Recursos (para ser limpiados por la señal)
IndexEntry *hash_table = NULL; 
FILE *index_file_disk = NULL; 
FILE *csv_file = NULL;
int server_fd = -1; 

// Globales para el índice de offsets
FILE *offset_index_file = NULL; // Archivo de offsets (para añadir nuevos)
uint64_t *line_offsets = NULL;  // Puntero al array de offsets en RAM (para búsqueda O(1))
long total_records = 0;         // Número total de líneas/registros

volatile sig_atomic_t keep_running = 1; 

// Función de limpieza de recursos
void cleanup_worker(int signum) {
    if (signum != 0) {
        printf("\n#WORKER -> Señal %d (Ctrl+C) recibida. Iniciando limpieza de recursos...\n", signum);
    } else {
        printf("\n#WORKER -> Terminación remota solicitada. Iniciando limpieza de recursos...\n");
    }
    
    keep_running = 0; 

    if (server_fd != -1) {
        printf("#WORKER -> Cerrando socket servidor...\n");
        close(server_fd); 
        server_fd = -1;
    }
    if (csv_file) {
        printf("#WORKER -> Cerrando archivo CSV...\n");
        fclose(csv_file);
        csv_file = NULL;
    }
    if (index_file_disk) {
        printf("#WORKER -> Cerrando archivo de índice Hash...\n");
        fclose(index_file_disk);
        index_file_disk = NULL;
    }

    if (offset_index_file) {
        printf("#WORKER -> Cerrando archivo de índice de Offsets...\n");
        fclose(offset_index_file);
        offset_index_file = NULL;
    }
    if (line_offsets) {
        printf("#WORKER -> Liberando memoria line_offsets...\n");
        free(line_offsets);
        line_offsets = NULL;
    }

    if (hash_table) {
        printf("#WORKER -> Liberando memoria hash_table...\n");
        free(hash_table);
        hash_table = NULL;
    }
    
    printf("#WORKER -> Limpieza completa. Terminando proceso.\n");
    exit(0);
}


// Función para inicializar el socket del servidor (listen)
int initialize_server_socket() {
    struct sockaddr_in address;
    int opt = 1;
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("#WORKER_SERVER -> Error al crear el socket");
        return -1;
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("#WORKER_SERVER -> setsockopt SO_REUSEADDR failed");
        close(server_fd);
        server_fd = -1;
        return -1;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(SOCKET_PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("#WORKER_SERVER -> Error en el bind");
        close(server_fd);
        server_fd = -1;
        return -1;
    }
    
    if (listen(server_fd, 5) < 0) {
        perror("#WORKER_SERVER -> Error en el listen");
        close(server_fd);
        server_fd = -1;
        return -1;
    }

    return server_fd;
}

// CMD_SEARCH: Busca la clave SMILES en el índice y CSV
void search_key(FILE *csv_file, FILE *index_file, const char *key, ServerResponse *response) {

    response->status = STATUS_NOT_FOUND;
    response->response_buffer[0] = '\0';
    
    int len = strlen(key);
    uint32_t hash = MurmurHash2(key, len, SEED);
    uint32_t index_pos = hash % TABLE_SIZE;

    if (!hash_table) {
        snprintf(response->response_buffer, sizeof(response->response_buffer), "Error interno: Tabla hash no cargada.");
        response->status = STATUS_ERROR;
        return;
    }

    IndexEntry current_entry = hash_table[index_pos];
    char temp_line[CSV_LINE_MAX]; 

    while (1) {
        if (current_entry.hash == hash) {
            
            if (fseek(csv_file, current_entry.offset, SEEK_SET) == 0 && 
                fgets(temp_line, sizeof(temp_line), csv_file) != NULL) 
            {
                char line_copy[CSV_LINE_MAX];
                strcpy(line_copy, temp_line); 
                
                strtok(line_copy, ","); 
                strtok(NULL, ","); 
                char *found_key = strtok(NULL, ","); 

                if (found_key) {
                    found_key[strcspn(found_key, "\r\n")] = 0;
                    while (found_key[0] == '"') { memmove(found_key, found_key + 1, strlen(found_key)); }
                    size_t len_found = strlen(found_key);
                    while (len_found > 0 && found_key[len_found - 1] == '"') { found_key[len_found - 1] = '\0'; len_found--; }
                    
                    if (strcmp(found_key, key) == 0) {
                        response->status = STATUS_OK;
                        snprintf(response->response_buffer, sizeof(response->response_buffer), 
                                 "Coincidencia encontrada:\n%s", temp_line); 
                        return;
                    }
                }
            }
        }
        
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
    
    snprintf(response->response_buffer, sizeof(response->response_buffer), 
             "Resultado: No se encontraron coincidencias para la clave SMILES '%s'.", key);
}


// CMD_WRITE: Escribe registro en CSV y actualiza AMBOS índices
void write_record(FILE *csv_file, FILE *index_file, const char *record, ServerResponse *response) {
    
    response->status = STATUS_ERROR;
    response->response_buffer[0] = '\0';
    
    // Preparar datos y extraer clave del nuevo registro
    char key_copy[CSV_LINE_MAX * 2];
    strncpy(key_copy, record, sizeof(key_copy) - 1);
    key_copy[sizeof(key_copy) - 1] = '\0';
    
    strtok(key_copy, ","); 
    strtok(NULL, ",");          
    char *smiles_key = strtok(NULL, ","); // product_smiles (CLAVE)
    
    if (!smiles_key) {
        snprintf(response->response_buffer, sizeof(response->response_buffer), "Error: El registro de entrada no contiene la clave SMILES (3ra columna).");
        return;
    }

    // Limpieza de clave
    smiles_key[strcspn(smiles_key, "\r\n")] = 0;
    while (smiles_key[0] == '"') { memmove(smiles_key, smiles_key + 1, strlen(smiles_key)); }
    size_t len_smiles = strlen(smiles_key);
    while (len_smiles > 0 && smiles_key[len_smiles - 1] == '"') { smiles_key[len_smiles - 1] = '\0'; len_smiles--; }
    
    // Escribir en el archivo CSV y obtener el offset
    fseek(csv_file, 0, SEEK_END);
    
    // Asegurar que la escritura es en una nueva línea
    if (ftell(csv_file) > 0) {
        fseek(csv_file, -1, SEEK_END); 
        char last_char;
        if (fread(&last_char, 1, 1, csv_file) == 1 && last_char != '\n') {
            fwrite("\n", 1, 1, csv_file);
        }
        fseek(csv_file, 0, SEEK_END); 
    }
    
    uint64_t offset = ftell(csv_file);
    
    char line_to_write[CSV_LINE_MAX * 2 + 2];
    snprintf(line_to_write, sizeof(line_to_write), "%s\n", record);
    size_t bytes_written = strlen(line_to_write);

    if (fwrite(line_to_write, 1, bytes_written, csv_file) != bytes_written) {
        snprintf(response->response_buffer, sizeof(response->response_buffer), "Error de I/O al escribir en el CSV.");
        return;
    }
    fflush(csv_file); 
    
    // Actualizar el índice de Offsets
    if (!offset_index_file) {
        snprintf(response->response_buffer, sizeof(response->response_buffer), "Error interno: Archivo de offsets no abierto.");
        return;
    }
    fseek(offset_index_file, 0, SEEK_END);
    fwrite(&offset, sizeof(uint64_t), 1, offset_index_file);
    fflush(offset_index_file);

    // Actualizar el array de offsets en RAM (por si hay otra escritura o lectura inmediata)
    total_records++;
    line_offsets = (uint64_t *)realloc(line_offsets, total_records * sizeof(uint64_t));
    if (!line_offsets) {
        perror("Error de realloc en line_offsets");
        snprintf(response->response_buffer, sizeof(response->response_buffer), "Error interno de memoria al expandir índice de offsets.");
        response->status = STATUS_ERROR;
        return;
    }
    line_offsets[total_records - 1] = offset;

    // Actualizar la tabla Hash (en RAM y Disco)
    int len = strlen(smiles_key);
    uint32_t hash = MurmurHash2(smiles_key, len, SEED);
    uint32_t index_pos = hash % TABLE_SIZE;
    
    IndexEntry current_bucket = hash_table[index_pos];

    if (current_bucket.hash == 0 && current_bucket.offset == 0) {
        IndexEntry new_entry = {hash, offset, -1};
        hash_table[index_pos] = new_entry;
        
        fseek(index_file, index_pos * sizeof(IndexEntry), SEEK_SET);
        fwrite(&new_entry, sizeof(IndexEntry), 1, index_file);
        
    } else {
        fseek(index_file, 0, SEEK_END);
        long file_pos_bytes = ftell(index_file);
        int32_t new_pos_index = (int32_t)(file_pos_bytes / sizeof(IndexEntry));
        
        IndexEntry new_entry = {hash, offset, current_bucket.next};
        fwrite(&new_entry, sizeof(IndexEntry), 1, index_file);
        
        hash_table[index_pos].next = new_pos_index;
        
        fseek(index_file, index_pos * sizeof(IndexEntry), SEEK_SET);
        fwrite(&hash_table[index_pos], sizeof(IndexEntry), 1, index_file);
    }
    fflush(index_file); 

    response->status = STATUS_OK;
    snprintf(response->response_buffer, sizeof(response->response_buffer), 
             "Registro añadido con éxito (Línea #%ld). Offset: %llu, Clave: '%s'. Ahora es buscable.", 
             total_records, (unsigned long long)offset, smiles_key);
}

// CMD_READ_BY_NUM: Lectura acelerada por número de línea (O(1))
void read_by_number(FILE *csv_file_ptr, int record_num, ServerResponse *response) {
    
    response->status = STATUS_NOT_FOUND;
    response->response_buffer[0] = '\0';
    
    int index = record_num - 1; // El índice es 0-based
    
    if (!line_offsets) {
        snprintf(response->response_buffer, sizeof(response->response_buffer), "Error interno: Índice de offsets no cargado.");
        response->status = STATUS_ERROR;
        return;
    }

    if (index < 0 || index >= total_records) {
        snprintf(response->response_buffer, sizeof(response->response_buffer), 
                 "Error: El número de registro %d está fuera del rango (1 a %ld).", record_num, total_records);
        response->status = STATUS_NOT_FOUND;
        return;
    }

    // BÚSQUEDA O(1): Obtener el offset de la RAM y saltar en el archivo
    uint64_t offset = line_offsets[index];
    
    if (fseek(csv_file_ptr, (long)offset, SEEK_SET) != 0) {
        snprintf(response->response_buffer, sizeof(response->response_buffer), 
                 "Error de I/O: Fallo al buscar el offset %llu.", (unsigned long long)offset);
        response->status = STATUS_ERROR;
        return;
    }

    char line[CSV_LINE_MAX];
    if (fgets(line, sizeof(line), csv_file_ptr) == NULL) {
        snprintf(response->response_buffer, sizeof(response->response_buffer), 
                 "Error de I/O: No se pudo leer el registro en el offset %llu.", (unsigned long long)offset);
        response->status = STATUS_ERROR;
        return;
    }
    
    response->status = STATUS_OK;
    snprintf(response->response_buffer, sizeof(response->response_buffer), 
             "Registro N° %d (Offset: %llu):\n%s", record_num, (unsigned long long)offset, line);
}


// Bucle Principal del Worker/Servidor
int run_worker(const char *csv_filename, const char *index_filename) {
    
    signal(SIGINT, cleanup_worker);
    signal(SIGTERM, cleanup_worker);
    
    // Cargar la tabla hash de disco a RAM 
    size_t table_size_bytes = (size_t)TABLE_SIZE * sizeof(IndexEntry);
    hash_table = (IndexEntry *)malloc(table_size_bytes);
    if (!hash_table) { perror("#WORKER -> Error de malloc para la tabla hash"); cleanup_worker(0); return EXIT_FAILURE; }
    
    // Abrir archivos
    index_file_disk = fopen(index_filename, "rb+"); 
    if (!index_file_disk) { perror("#WORKER -> Error abriendo hash_index.bin (Debe existir)"); cleanup_worker(0); return EXIT_FAILURE; }
    
    if (fread(hash_table, sizeof(IndexEntry), TABLE_SIZE, index_file_disk) != TABLE_SIZE) { 
        fprintf(stderr, "#WORKER -> Advertencia: Índice Hash inicial incompleto.\n");
    }
    printf("#WORKER -> Tabla hash cargada en RAM (%.3f MB).", (float)table_size_bytes / (1024 * 1024));
    
    csv_file = fopen(csv_filename, "rb+");
    if (!csv_file) { perror("#WORKER -> Error abriendo CSV"); cleanup_worker(0); return EXIT_FAILURE; }
    
    // Cargar el Índice de Offsets a RAM
    offset_index_file = fopen(OFFSET_INDEX_FILENAME, "rb+");
    if (!offset_index_file) { 
        perror("#WORKER -> Error abriendo line_offsets.bin (Debe existir)"); 
        cleanup_worker(0); 
        return EXIT_FAILURE; 
    }

    // Determinar el tamaño y el número de registros
    fseek(offset_index_file, 0, SEEK_END);
    long file_size = ftell(offset_index_file);
    rewind(offset_index_file);

    total_records = file_size / sizeof(uint64_t);
    line_offsets = (uint64_t *)malloc(file_size);

    if (!line_offsets) {
        perror("#WORKER -> Error de malloc para line_offsets");
        cleanup_worker(0); 
        return EXIT_FAILURE; 
    }

    if (fread(line_offsets, sizeof(uint64_t), total_records, offset_index_file) != total_records) {
        fprintf(stderr, "#WORKER -> Error leyendo índice de offsets (total esperado: %ld).\n", total_records);
    }
    printf(" Índice de offsets cargado (%ld registros). Archivos abiertos.\n", total_records);

    // Inicialización del Socket Servidor 
    server_fd = initialize_server_socket();
    if (server_fd < 0) {
        cleanup_worker(0);
        return EXIT_FAILURE;
    }
    printf("#WORKER -> Servidor listo. Esperando conexiones...\n");

    // Bucle principal de manejo de conexiones 
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int new_socket;
    ClientMessage client_request;
    ServerResponse server_response;

    while (keep_running) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
            if (keep_running == 0) break; 
            perror("#WORKER_SERVER -> Error en accept");
            continue;
        }
        
        printf("\n#WORKER_SERVER -> Conexión aceptada desde %s.\n", inet_ntoa(address.sin_addr));

        while (keep_running) {
            ssize_t valread = recv(new_socket, &client_request, sizeof(ClientMessage), 0);
            
            if (valread <= 0) {
                printf("#WORKER_SERVER -> Cliente desconectado. Cerrando socket.\n");
                break;
            }
            
            clock_t start = clock();
            memset(&server_response, 0, sizeof(ServerResponse)); 

            if (client_request.command == CMD_SEARCH) {
                // CMD_SEARCH
                search_key(csv_file, index_file_disk, client_request.key_or_record, &server_response);
                
            } else if (client_request.command == CMD_WRITE) {
                // CMD_WRITE
                write_record(csv_file, index_file_disk, client_request.key_or_record, &server_response);
                
            } else if (client_request.command == CMD_READ_BY_NUM) {
                // CMD_READ_BY_NUM
                read_by_number(csv_file, client_request.record_number, &server_response);
                
            } else if (client_request.command == CMD_EXIT) {
                printf("#WORKER -> Comando CMD_EXIT recibido. Apagando servidor remotamente.\n");
                keep_running = 0; 
                break; 
                
            } else {
                server_response.status = STATUS_ERROR;
                snprintf(server_response.response_buffer, sizeof(server_response.response_buffer), "Comando desconocido: %d", client_request.command);
            }
            
            clock_t end = clock();
            double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
            printf("#WORKER -> Tarea finalizada en %.5f segundos.\n", time_spent);

            send(new_socket, &server_response, sizeof(ServerResponse), 0);
        }

        if (new_socket != -1) close(new_socket); 
    }

    cleanup_worker(0); 
    return EXIT_SUCCESS;
}