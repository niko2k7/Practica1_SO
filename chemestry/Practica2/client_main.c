#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "definitions.h" 

// Declaración de la función de UI que se encuentra en ui_process.c
void run_ui(const char *server_ip);

int main(int argc, char *argv[]) {
    
    char server_ip[SERVER_IP_MAX];

    printf("--- Iniciador del Cliente de búsqueda ---\n");
    printf("Ingrese la IP del servidor Worker (ej: 127.0.0.1 o IP_de_otra_máquina): ");
    
    // Lectura de la IP del servidor
    if (fgets(server_ip, sizeof(server_ip), stdin) == NULL) {
        fprintf(stderr, "Error al leer la IP.\n");
        return EXIT_FAILURE;
    }
    
    // Limpieza de salto de línea
    server_ip[strcspn(server_ip, "\r\n")] = 0;
    
    if (server_ip[0] == '\0') {
        fprintf(stderr, "La IP no puede estar vacía.\n");
        return EXIT_FAILURE;
    }

    printf("#CLIENT_MAIN -> Conectándose a %s:%d...\n", server_ip, SOCKET_PORT);
    
    run_ui(server_ip); 

    return 0;
}