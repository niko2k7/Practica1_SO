#include <stdint.h>
#include <unistd.h> 
#include <sys/wait.h> 

// Definiciones de dataset y hash
#define DATASET_FILENAME "../dataset_1_1GB.csv"
#define INDEX_FILENAME "hash_index.bin"
#define OFFSET_INDEX_FILENAME "line_offsets.bin" 
#define CSV_LINE_MAX 2048 // Máximo de caracteres por línea
#define SEARCH_KEY_MAX 256
#define TABLE_SIZE 997 // Tamaño para cumplir el requisito <= 10MB 
#define SEED 0xDEADBEEF

// Estructuras de datos (Para el índice)
typedef struct {
    uint32_t hash;       // Hash de 32 bits de la clave
    uint64_t offset;     // Posición en bytes de la línea en el CSV
    int32_t next;        // Índice de la siguiente entrada en el archivo de índice (-1 si es la última)
} IndexEntry;

// Definiciones de comunicación (Sockets)
#define SOCKET_PORT 8080        // Puerto de escucha del Worker/Servidor
#define SERVER_IP_MAX 32        // Máximo de caracteres para la dirección IP

// Comandos de comunicación (Cliente -> Servidor)
#define CMD_NONE 0
#define CMD_SEARCH 1            // Búsqueda por clave (product_smiles)
#define CMD_WRITE 2             // Escritura de un nuevo registro
#define CMD_READ_BY_NUM 3       // Lectura por número de registro (línea N)
#define CMD_EXIT 4              // Salir del programa

// Códigos de estado de respuesta (Servidor -> Cliente)
#define STATUS_OK 0             // Operación exitosa
#define STATUS_NOT_FOUND 1      // Dato no encontrado
#define STATUS_ERROR 2          // Error de Servidor/IO

// Estructuras de comunicación (Sockets)
// Mensaje enviado por el Cliente (UI) al Servidor (Worker)
typedef struct {
    int command;                                // Comando a ejecutar
    int record_number;                          // Usado por CMD_READ_BY_NUM
    char key_or_record[CSV_LINE_MAX * 2];       // Clave de búsqueda o línea completa del registro a escribir
} ClientMessage; 

// Respuesta enviada por el Servidor (Worker) al Cliente (UI)
typedef struct {
    int status;                                 // Código de estado
    char response_buffer[CSV_LINE_MAX * 4];     // Buffer para el resultado
} ServerResponse; 

// Declaración de la Función Hash 
uint32_t MurmurHash2(const void * key, int len, uint32_t seed);