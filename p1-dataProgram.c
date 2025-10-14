#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define TABLE_SIZE 5000003     // número primo grande (número de buckets)
#define BUCKET_SIZE 16         // bytes por entrada en el archivo índice
#define SEED 0x9747b28c        // semilla del hash
#define CHAR_LENGTH 15


typedef struct {
    uint32_t hash;     // hash del Movie_Name
    uint64_t offset;   // posición (en bytes) en el CSV
    int32_t next;      // siguiente colisión (-1 si no hay)
} IndexEntry;


// MurmurHash2
uint32_t MurmurHash2(const void * key, int len, uint32_t seed)
{
    const uint32_t m = 0x5bd1e995;
    const int r = 24;

    uint32_t h = seed ^ len;

    const unsigned char * data = (const unsigned char *)key;

    while (len >= 4)
    {
        uint32_t k = *(uint32_t *)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    switch (len)
    {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0];
            h *= m;
    };

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

void build_index(const char *csv_filename, const char *index_filename){
    // Se abre el dataset, en modo lectura
    FILE *csv = fopen(csv_filename, "r");
    if(!csv){
        perror("Error abirendo el csv.");
        exit(1);
    }

    // Se abre (y crea si no existe) el hash_index, en modo escritura en binario
    FILE *index = fopen(index_filename, "wb+");
    if(!index){
        perror("Error abirendo el csv.");
        fclose(csv);
        exit(1);
    }

    // Llenar la tabla con structs IndexEntry vacías
    IndexEntry empty = {0, 0, -1};
    for(int i=0; i<TABLE_SIZE; i++){
        fwrite(&empty, sizeof(IndexEntry), 1, index);
    }

    char line[8192]; // Buffer para la linea completa
    char *movie; 
    uint64_t offset;

    //Encabezado
    fgets(line, sizeof(line), csv);
    long colisiones = 0;

    while ((offset = ftell(csv)), fgets(line, sizeof(line), csv)) {
        // Copiamos la línea para extraer el nombre
        char temp[8192];
        strcpy(temp, line);

        // Se extrae el nombre de la película (2ra columna) ---
        char *token = strtok(temp, ","); // id
        movie = strtok(NULL, ",");       // title

        if (!movie) continue;

        // Quitar salto de línea
        movie[strcspn(movie, "\r\n")] = 0;

        // 🔹 Quitar comillas dobles si existen
        if (movie[0] == '"') {
            // Desplazar una posición hacia la izquierda (remueve la comilla inicial)
            memmove(movie, movie + 1, strlen(movie));
        }
        if (movie[strlen(movie) - 1] == '"') {
            // Remover la comilla final
            movie[strlen(movie) - 1] = '\0';
        }

        // Usamos solo 10 caracteres
        int len = strlen(movie);
        if (len>CHAR_LENGTH) len=CHAR_LENGTH;

        // Hacemos hash
        uint32_t hash = MurmurHash2(movie, len, SEED);
        // Calculamos la posicion en la tabla
        uint32_t index_pos = hash % TABLE_SIZE;

        printf("Movie:%s |Hash:%u |Index:%u \n", movie, hash, index_pos);
        
        // Leer la entrada existente del bucket principal
        IndexEntry current_bucket;
        fseek(index, index_pos * sizeof(IndexEntry), SEEK_SET);
        fread(&current_bucket, sizeof(IndexEntry), 1, index);

        // Si el bucket está vacío, se inserta directamente
        if (current_bucket.next == -1 && current_bucket.hash == 0 && current_bucket.offset == 0) {
            IndexEntry new_entry = {hash, offset, -1,};
            fseek(index, index_pos * sizeof(IndexEntry), SEEK_SET);
            fwrite(&new_entry, sizeof(IndexEntry), 1, index);
        } else {
            colisiones++;

            // Guardamos la posición actual del bucket principal
            int32_t old_next = current_bucket.next;

            // Creamos la nueva entrada, que apunta al anterior 'next'
            IndexEntry new_entry = {hash, offset, old_next};

            // Escribimos al final del archivo
            fseek(index, 0, SEEK_END);
            long new_pos = ftell(index) / sizeof(IndexEntry);
            fwrite(&new_entry, sizeof(IndexEntry), 1, index);

            // Actualizamos el bucket principal para que su next apunte al nuevo registro
            current_bucket.next = (int32_t)new_pos;
            fseek(index, index_pos * sizeof(IndexEntry), SEEK_SET);
            fwrite(&current_bucket, sizeof(IndexEntry), 1, index);
        }
    }

    fclose(csv);
    fclose(index);
    printf("Índice hash creado correctamente.\n");
    printf("Colisiones:%lu \n", colisiones);
}


void search_movie(const char *csv_filename, const char *index_filename, const char *movie_name) {

    FILE *csv = fopen(csv_filename, "r");
    if (!csv) {
        perror("Error abriendo el archivo CSV");
        return;
    }

    FILE *index = fopen(index_filename, "rb");
    if (!index) {
        perror("Error abriendo el archivo de índice");
        fclose(csv);
        return;
    }
    // Calcular hash
    int len = strlen(movie_name);
    if (len > CHAR_LENGTH) len = CHAR_LENGTH;

    uint32_t hash = MurmurHash2(movie_name, len, SEED);
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
                printf("\nPelícula encontrada:\n%s\n", line);
            } else {
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

    printf("NA (película no encontrada)\n");

    fclose(csv);
    fclose(index);
}

int main(){
    // build_index("DataSet/all_movies_heavy.csv", "DataSet/hash_index.bin");

    char movie_name[100];
    printf("Ingrese el nombre de la película a buscar: ");
    fgets(movie_name, sizeof(movie_name), stdin);
    movie_name[strcspn(movie_name, "\r\n")] = 0; // quitar salto

    search_movie("DataSet/all_movies_heavy.csv", "DataSet/hash_index.bin", movie_name);

    return 0;
}