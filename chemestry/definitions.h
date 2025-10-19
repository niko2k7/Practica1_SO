// definitions.h

#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <stdint.h>
#include <unistd.h>
#include <errno.h>

// --- CONSTANTES DEL SISTEMA ---

// Tamaño de la tabla hash principal. 
// Aprox. 9.6 MB con padding de 24 bytes/slot
#define TABLE_SIZE 419407

// Semilla para la función MurmurHash2
#define SEED 0x9747b28c

// Longitud máxima de una línea en el CSV y de la clave de búsqueda.
#define CSV_LINE_MAX 8192
#define SEARCH_KEY_MAX 256

// --- DEFINICIONES DE ARCHIVOS ---

#define DATASET_FILENAME "dataset_1_1GB.csv"
#define INDEX_FILENAME "hash_index.bin"

// --- COMUNICACIÓN ENTRE PROCESOS (FIFOs) ---

#define FIFO_UI_TO_WORKER "/tmp/fifo_ui_to_worker"
#define FIFO_WORKER_TO_UI "/tmp/fifo_worker_to_ui"

// --- ESTRUCTURAS DE DATOS ---

// Estructura de la entrada del índice hash (16 bytes)
typedef struct {
    uint32_t hash;     // 4 bytes: Hash del product_smiles
    uint64_t offset;   // 8 bytes: Posición (en bytes) en el archivo CSV
    int32_t next;      // 4 bytes: Puntero a la siguiente colisión (-1 si no hay).
} IndexEntry;


// Tipos de comandos para el protocolo de comunicación FIFO
typedef enum {
    CMD_SEARCH,
    CMD_EXIT
} CommandType;

// Mensaje transferido a través de la tubería FIFO
typedef struct {
    CommandType command;
    // Criterio 1: product_smiles (la clave de hashing)
    char search_key1[SEARCH_KEY_MAX]; 
} PipeMessage;

// --- PROTOTIPO DE FUNCIÓN HASH ---

// Prototipo de la función MurmurHash2, ahora en el encabezado principal
uint32_t MurmurHash2(const void * key, int len, uint32_t seed);

#endif // DEFINITIONS_H