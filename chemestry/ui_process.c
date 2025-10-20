#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h> 
#include <unistd.h>
#include "definitions.h"

// Clave de búsqueda 
char current_smiles[SEARCH_KEY_MAX] = ""; 

void show_menu(const char *smiles) {
    printf("\n--- Sistema de búsqueda de moléculas ---\n");
    printf("1. Ingresar criterio de búsqueda (product_smiles):\n   Clave actual: %s\n", smiles[0] == '\0' ? "[VACÍO]" : smiles);
    printf("2. Realizar búsqueda\n");
    printf("3. Salir\n");
    printf("Selección: ");
}

void run_ui(SharedMessage *shm_ptr) {
    
    // Configuración de memoria compartida. Usar el puntero que ya se recibió
    shm_ptr->ui_pid = getpid();
    
    printf("#UI -> Interfaz iniciada y conectada a Worker.\n");

    int choice;
    while (1) {
        show_menu(current_smiles); 
        
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }
        
        while (getchar() != '\n');

        if (choice == 1) {
            // Ingreso de molécula
            printf("Ingrese product_smiles (ej: O(C(=O)C2(CC1...)): ");
            if (fgets(current_smiles, sizeof(current_smiles), stdin)) {
                current_smiles[strcspn(current_smiles, "\r\n")] = 0;
            }
        } else if (choice == 2) {
            // Realizar búsqueda
            if (current_smiles[0] == '\0') {
                printf("# Debe ingresar el criterio (product_smiles) primero.\n");
                continue;
            }
            
            // Enviar comando y datos (con shm_ptr)
            if (shm_ptr->command != CMD_NONE) {
                printf("El Worker está ocupado. Intente de nuevo.\n");
                continue;
            }
            
            strncpy(shm_ptr->search_key1, current_smiles, sizeof(shm_ptr->search_key1) - 1);
            shm_ptr->command = CMD_SEARCH; // Señal de inicio de búsqueda

            printf("\nBuscando '%s'. Esperando resultado del Worker...\n", current_smiles);
            
            // Recibir respuesta (esperar hasta que el Worker acabe)
            while (shm_ptr->command == CMD_SEARCH) {
                usleep(10000); // Esperar 10000μs -> 10ms (polling)
            }

            // Leer resultado
            if (shm_ptr->response_buffer[0] != '\0') {
                printf("%s", shm_ptr->response_buffer);
                shm_ptr->response_buffer[0] = '\0'; // Limpiar buffer después de leer
            }

        } else if (choice == 3) {
            // Salir
            shm_ptr->command = CMD_EXIT;
            printf("Comando de salida enviado al Worker. Terminando...\n");
            break;
        } else {
            printf("Opción no válida. Intente de nuevo.\n");
        }
    }
}