#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "definitions.h" 

// Punteros al archivo del índice binario
FILE *index_file_disk = NULL;
FILE *csv_file = NULL;

// Función de limpieza de recursos
void cleanup(IndexEntry *table, FILE *index_file, FILE *csv_file_ptr) {
    if (table) free(table);
    if (index_file) fclose(index_file);
    if (csv_file_ptr) fclose(csv_file_ptr);
}

int main() {
    
    printf("--- Constructor de índice (Hash y Offset) ---\n");
    
    IndexEntry *hash_table = NULL;
    
    // Inicialización de la tabla Hash en RAM
    size_t table_size_bytes = (size_t)TABLE_SIZE * sizeof(IndexEntry);
    hash_table = (IndexEntry *)calloc(TABLE_SIZE, sizeof(IndexEntry)); // calloc inicializa a 0 para detección de bucket vacío
    if (!hash_table) {
        perror("Error de malloc para la tabla hash");
        return EXIT_FAILURE;
    }

    // Apertura de archivos
    csv_file = fopen(DATASET_FILENAME, "r");
    if (!csv_file) {
        perror("Error abriendo el archivo CSV");
        cleanup(hash_table, index_file_disk, csv_file);
        return EXIT_FAILURE;
    }

    index_file_disk = fopen(INDEX_FILENAME, "wb+");
    if (!index_file_disk) {
        perror("Error abriendo/creando hash_index.bin");
        cleanup(hash_table, index_file_disk, csv_file);
        return EXIT_FAILURE;
    }
    
    // Archivo para el índice de offsets
    FILE *offset_index_file = fopen(OFFSET_INDEX_FILENAME, "wb");
    if (!offset_index_file) {
        perror("Error abriendo/creando line_offsets.bin");
        cleanup(hash_table, index_file_disk, csv_file);
        return EXIT_FAILURE;
    }

    // Lectura y construcción de Índices
    char line[CSV_LINE_MAX];
    uint64_t current_offset = 0;
    long record_count = 0;
    
    printf("Comenzando el escaneo del CSV...\n");

    while (fgets(line, sizeof(line), csv_file) != NULL) {
        
        // Escribir el offset actual al índice de offsets
        fwrite(&current_offset, sizeof(uint64_t), 1, offset_index_file);
        
        record_count++;

        // Extracción de la clave (product_smiles, 3ra columna)
        char line_copy[CSV_LINE_MAX];
        strncpy(line_copy, line, sizeof(line_copy) - 1);
        line_copy[sizeof(line_copy) - 1] = '\0';
        
        // El CSV usa comillas triples. Saltamos las dos primeras columnas.
        strtok(line_copy, ","); // product_name
        strtok(NULL, ",");      // product_hashisy
        char *smiles_key = strtok(NULL, ","); // product_smiles (CLAVE)
        
        if (smiles_key) {
            // Limpieza de clave (eliminar comillas triples y salto de línea)
            smiles_key[strcspn(smiles_key, "\r\n")] = 0;
            while (smiles_key[0] == '"') { memmove(smiles_key, smiles_key + 1, strlen(smiles_key)); }
            size_t len_smiles = strlen(smiles_key);
            while (len_smiles > 0 && smiles_key[len_smiles - 1] == '"') { smiles_key[len_smiles - 1] = '\0'; len_smiles--; }
            
            // Cálculo de Hash
            int len = strlen(smiles_key);
            uint32_t hash = MurmurHash2(smiles_key, len, SEED);
            uint32_t index_pos = hash % TABLE_SIZE;
            
            // Inserción en la Tabla Hash 
            if (hash_table[index_pos].hash == 0 && hash_table[index_pos].offset == 0) {
                // Nuevo bucket
                hash_table[index_pos].hash = hash;
                hash_table[index_pos].offset = current_offset;
                hash_table[index_pos].next = -1;
            } else {
                // Colisión (desbordamiento)
                IndexEntry new_entry = {hash, current_offset, hash_table[index_pos].next};
                
                fseek(index_file_disk, 0, SEEK_END);
                long file_pos_bytes = ftell(index_file_disk);
                int32_t new_pos_index = (int32_t)(file_pos_bytes / sizeof(IndexEntry));
                
                fwrite(&new_entry, sizeof(IndexEntry), 1, index_file_disk);
                
                // Actualizar el puntero del bucket principal
                hash_table[index_pos].next = new_pos_index;
            }
        }

        // Actualizar el offset para la siguiente línea
        current_offset += strlen(line);
    }
    
    // Escritura final de la Tabla Hash principal 
    rewind(index_file_disk); // Devolver el puntero del archivo al inicio para escribir la tabla principal
    fwrite(hash_table, sizeof(IndexEntry), TABLE_SIZE, index_file_disk);

    printf("Escaneo completado. %ld registros procesados.\n", record_count);
    printf("Tabla hash principal escrita. Tamaño del índice binario: %ld bytes.\n", ftell(index_file_disk));
    
    // Cierre y limpieza
    fclose(offset_index_file); 
    cleanup(hash_table, index_file_disk, csv_file);

    return EXIT_SUCCESS;
}