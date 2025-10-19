#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "definitions.h"

// Variable global para la clave de búsqueda, necesaria para persistir entre llamadas a show_menu
char current_smiles[SEARCH_KEY_MAX] = ""; 

void show_menu() {
    printf("\n--- Sistema de Búsqueda de Moléculas (1.1GB) ---\n");
    printf("1. Ingresar Criterio de Búsqueda (product_smiles):\n   Clave actual: %s\n", current_smiles[0] == '\0' ? "[VACÍO]" : current_smiles);
    printf("2. Realizar Búsqueda\n");
    printf("3. Salir\n");
    printf("Selección: ");
}

void run_ui() {
    
    // NOTA: Los FIFOs se abren y cierran DENTRO de los bloques case para forzar el flujo
    // de comunicación y evitar el bloqueo.
    printf("[UI] Interfaz iniciada. Lista para comunicarse con el Worker.\n");

    int choice;
    while (1) {
        show_menu();
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }
        
        while (getchar() != '\n');

        if (choice == 1) {
            printf("Ingrese product_smiles (ej: O(C(=O)C2(CC1...)): ");
            if (fgets(current_smiles, sizeof(current_smiles), stdin)) {
                current_smiles[strcspn(current_smiles, "\r\n")] = 0;
            }
        } else if (choice == 2) {
            // REALIZAR BÚSQUEDA
            if (current_smiles[0] == '\0') {
                printf("⚠️ Debe ingresar el Criterio (product_smiles) primero.\n");
                continue;
            }
            
            // 1. ABRIR FD_WRITE (Bloqueante si el Worker no está leyendo)
            int fd_write = open(FIFO_UI_TO_WORKER, O_WRONLY);
            if (fd_write == -1) { perror("[UI] Error abriendo FIFO de escritura."); continue; }

            PipeMessage message = {0}; 
            message.command = CMD_SEARCH;
            strncpy(message.search_key1, current_smiles, sizeof(message.search_key1) - 1);

            // 2. ENVIAR MENSAJE
            write(fd_write, &message, sizeof(PipeMessage));
            close(fd_write); // CERRAMOS: Permite que el Worker lo abra después de leer

            printf("\nBuscando '%s'. Esperando resultado del Worker...\n", current_smiles);
            
            // 3. ABRIR FD_READ (Bloqueante, espera a que el Worker escriba)
            int fd_read = open(FIFO_WORKER_TO_UI, O_RDONLY);
            if (fd_read == -1) { perror("[UI] Error abriendo FIFO de lectura."); continue; }

            // 4. RECIBIR RESPUESTA
            char response_buffer[CSV_LINE_MAX * 2];
            ssize_t bytes_read;

            // Leemos hasta que el Worker cierre su fd_write (final de mensaje)
            while ((bytes_read = read(fd_read, response_buffer, sizeof(response_buffer) - 1)) > 0) {
                 response_buffer[bytes_read] = '\0';
                 printf("%s", response_buffer);
                 
                 if (strstr(response_buffer, "finalizada")) break;
            }
            
            close(fd_read); // CERRAMOS: Desbloquea el Worker
            
            if (bytes_read == 0) {
                // Si el Worker cerró el FIFO antes de enviar el mensaje completo
                printf("❌ La conexión con el Worker se cerró prematuramente.\n");
            } else if (bytes_read < 0) {
                perror("Error leyendo respuesta del Worker");
            }

        } else if (choice == 3) {
            // SALIR
            int fd_write = open(FIFO_UI_TO_WORKER, O_WRONLY | O_NONBLOCK);
            if (fd_write != -1) {
                PipeMessage message = {0}; 
                message.command = CMD_EXIT;
                write(fd_write, &message, sizeof(PipeMessage));
                close(fd_write);
            }
            printf("Comando de salida enviado al Worker. Terminando...\n");
            break;
        } else {
            printf("Opción no válida. Intente de nuevo.\n");
        }
    }
}