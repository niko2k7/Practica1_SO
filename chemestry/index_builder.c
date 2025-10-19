// index_builder.c

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "definitions.h" 

// Prototipo local para la función de construcción
void build_index(const char *csv_filename, const char *index_filename);

/**
 * @brief Lee el archivo CSV y construye el índice hash binario en disco.
 */
void build_index(const char *csv_filename, const char *index_filename){
    
    FILE *csv = fopen(csv_filename, "r");
    if(!csv){
        perror("Error abriendo el dataset CSV. Asegúrese de que el archivo exista");
        exit(1);
    }

    FILE *index = fopen(index_filename, "wb+");
    if(!index){
        perror("Error abriendo el archivo de índice binario.");
        fclose(csv);
        exit(1);
    }

    // 1. Llenar la tabla principal con structs IndexEntry vacías
    IndexEntry empty = {0, 0, -1};
    printf("Inicializando tabla de índice de %u entradas (Ajustada para <= 10MB)...\n", TABLE_SIZE);
    
    for(uint32_t i=0; i<TABLE_SIZE; i++){
        fwrite(&empty, sizeof(IndexEntry), 1, index);
    }

    char line[CSV_LINE_MAX];
    char *smiles_key; 
    uint64_t offset;
    long colisiones = 0;

    // 2. Saltar el encabezado
    fgets(line, sizeof(line), csv);

    printf("Iniciando la construcción del índice. Esto puede tomar varios minutos...\n");

    while (1) {
        // Almacenamos el offset ANTES de leer la línea
        offset = ftell(csv); 
        if (fgets(line, sizeof(line), csv) == NULL) break;

        char temp[CSV_LINE_MAX];
        strcpy(temp, line);

        // --- Extracción de la clave: product_smiles (3ra columna) ---
        strtok(temp, ","); // 1ra columna (product_name)
        strtok(NULL, ","); // 2da columna (product_hashisy)
        smiles_key = strtok(NULL, ","); // 3ra columna (product_smiles - CLAVE)

        if (!smiles_key) continue;

        // 3. Limpieza de la clave

        // A. Quitar salto de línea
        smiles_key[strcspn(smiles_key, "\r\n")] = 0;

        // B. Limpieza robusta de comillas iniciales (maneja """ o "")
        while (smiles_key[0] == '"') {
            memmove(smiles_key, smiles_key + 1, strlen(smiles_key));
        }

        // C. Limpieza robusta de comillas finales
        size_t len_smiles = strlen(smiles_key);
        while (len_smiles > 0 && smiles_key[len_smiles - 1] == '"') {
            smiles_key[len_smiles - 1] = '\0';
            len_smiles--;
        }
        
        // 4. Hashing e inserción
        int len = strlen(smiles_key);
        uint32_t hash = MurmurHash2(smiles_key, len, SEED);
        uint32_t index_pos = hash % TABLE_SIZE;
        
        // Lógica de Colisión (Encadenamiento en el Archivo de Índice)
        IndexEntry current_bucket;
        fseek(index, index_pos * sizeof(IndexEntry), SEEK_SET);
        fread(&current_bucket, sizeof(IndexEntry), 1, index);

        if (current_bucket.hash == 0 && current_bucket.offset == 0) {
            IndexEntry new_entry = {hash, offset, -1};
            fseek(index, index_pos * sizeof(IndexEntry), SEEK_SET);
            fwrite(&new_entry, sizeof(IndexEntry), 1, index);
        } else {
            colisiones++;

            int32_t old_next = current_bucket.next;
            IndexEntry new_entry = {hash, offset, old_next};

            fseek(index, 0, SEEK_END);
            long file_pos_bytes = ftell(index);
            int32_t new_pos_index = (int32_t)(file_pos_bytes / sizeof(IndexEntry));
            
            fwrite(&new_entry, sizeof(IndexEntry), 1, index);

            current_bucket.next = new_pos_index;
            fseek(index, index_pos * sizeof(IndexEntry), SEEK_SET);
            fwrite(&current_bucket, sizeof(IndexEntry), 1, index);
        }
    }

    fclose(csv);
    fclose(index);
    printf("✅ Índice hash creado correctamente: %s\n", index_filename);
    printf("   Entradas totales en la tabla inicial: %u\n", TABLE_SIZE);
    printf("   Colisiones (Entradas de desbordamiento): %lu\n", colisiones);
}

int main(){
    build_index(DATASET_FILENAME, INDEX_FILENAME);
    return 0;
}