#include <stdint.h>
#include <unistd.h> 
#include <sys/wait.h> 

// Definiciones de dataset y hash
#define DATASET_FILENAME "dataset_1_1GB.csv"
#define INDEX_FILENAME "hash_index.bin"
#define CSV_LINE_MAX 2048
#define SEARCH_KEY_MAX 256
// Tamaño ajustado a ~9.6 MB para cumplir el requisito <= 10MB 
#define TABLE_SIZE 419407 
#define SEED 0xDEADBEEF

// Estructuras de datos
typedef struct {
    uint32_t hash;       // Hash de 32 bits de la clave
    uint64_t offset;     // Posición en bytes de la línea en el CSV
    int32_t next;        // Índice de la siguiente entrada en el archivo de índice (-1 si es la última)
} IndexEntry;

// Definiciones de comunicación. Memoria compartida
#define SHM_KEY_DATA 12345      // Clave para la memoria compartida de mensajes

// Comandos de sincronización
#define CMD_NONE 0
#define CMD_SEARCH 1
#define CMD_EXIT 2

// Estructura de comunicación entre procesos (UI, Worker)
typedef struct {
    int command;                            // Comando a ejecutar (CMD_SEARCH, CMD_EXIT)
    char search_key1[SEARCH_KEY_MAX];       // Clave de búsqueda
    char response_buffer[CSV_LINE_MAX * 2]; // Buffer para la respuesta del Worker
    pid_t worker_pid;
    pid_t ui_pid;
} SharedMessage; 

// Función Hash 
uint32_t MurmurHash2(const void * key, int len, uint32_t seed);