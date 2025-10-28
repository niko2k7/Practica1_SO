#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "definitions.h" 

// Clave de búsqueda actual, registro a escribir, y número de registro
char current_search_key[SEARCH_KEY_MAX] = ""; 
char current_record_to_write[CSV_LINE_MAX * 2] = "";
int client_socket = -1; // Socket global para la conexión

// *** Funciones Auxiliares 

// Función para conectar el socket del cliente al servidor
int connect_to_server(const char *ip) {
    int sock;
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("#UI_CLIENT -> Error al crear socket");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SOCKET_PORT);

    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "#UI_CLIENT -> Dirección IP inválida/no soportada\n");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("#UI_CLIENT -> Falló la conexión al servidor");
        close(sock);
        return -1;
    }
    
    printf("#UI_CLIENT -> Conexión exitosa con el Servidor.\n");
    return sock;
}

// Muestra el menú de la interfaz
void show_menu() {
    printf("\n--- Sistema de búsqueda de moléculas ---\n");
    printf("1. Búsqueda por SMILES (Clave actual: %s)\n", current_search_key[0] == '\0' ? "[VACÍO]" : current_search_key);
    printf("2. Escribir un nuevo registro\n");
    printf("3. Leer a partir de número de registro\n");
    printf("4. Salir\n");
    printf("Selección: ");
}

// Envía el mensaje y espera la respuesta del servidor
void send_and_receive(ClientMessage *msg, ServerResponse *res) {
    if (client_socket < 0) {
        fprintf(stderr, "#UI_CLIENT -> Error: Socket no conectado.\n");
        return;
    }
    
    // Envía el comando y los datos
    if (send(client_socket, msg, sizeof(ClientMessage), 0) < 0) {
        perror("#UI_CLIENT -> Error al enviar datos al servidor");
        return;
    }
    
    // Espera la respuesta
    ssize_t bytes_read = recv(client_socket, res, sizeof(ServerResponse), 0);
    if (bytes_read < 0) {
        perror("#UI_CLIENT -> Error al recibir respuesta del servidor");
        return;
    }
    if (bytes_read == 0) {
        fprintf(stderr, "#UI_CLIENT -> Servidor cerró la conexión inesperadamente. Reconecte.\n");
        client_socket = -1;
        return;
    }

    // Muestra la respuesta
    printf("\n--- Respuesta del Servidor ---\n");
    if (res->status == STATUS_OK) {
        printf("Estado: OK\n");
    } else if (res->status == STATUS_NOT_FOUND) {
        printf("Estado: NO ENCONTRADO\n");
    } else {
        printf("Estado: ERROR DE SERVIDOR\n");
    }
    printf("%s\n", res->response_buffer);
}

// Nueva función para solicitar cada atributo
void request_attribute(const char *prompt, const char *default_value, char *buffer, size_t buffer_size) {
    char response[5];
    printf("Desea ingresar valor para %s? (s/n, Defecto: %s): ", prompt, default_value);
    
    if (fgets(response, sizeof(response), stdin) && (response[0] == 's' || response[0] == 'S')) {
        printf("Ingrese %s: ", prompt);
        if (fgets(buffer, buffer_size, stdin)) {
            // Eliminar salto de línea
            buffer[strcspn(buffer, "\r\n")] = 0;
        } else {
            // Error en lectura, usar defecto
            strncpy(buffer, default_value, buffer_size - 1);
            buffer[buffer_size - 1] = '\0';
        }
    } else {
        // Usar valor por defecto
        strncpy(buffer, default_value, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    }
}

// *** Bucle Principal de la UI 

void run_ui(const char *server_ip) {
    
    // Establecer conexión inicial
    client_socket = connect_to_server(server_ip);
    if (client_socket < 0) {
        fprintf(stderr, "#UI_CLIENT -> No se pudo establecer la conexión. Terminando UI.\n");
        return;
    }

    int choice;
    ClientMessage request;
    ServerResponse response;

    // Buffer temporal para los 24 atributos
    char temp_attrs[24][CSV_LINE_MAX / 2];
    
    // Nombres y valores por defecto para cada atributo
    const char *attr_names[] = {
        "product_name", "product_hashisy", "product_smiles", "formula", "weight", "rotbonds", 
        "h_donors", "h_acceptors", "complexity", "product_qed_score", "hydrogen_bond_center_count", 
        "heavy_atoms", "ring_system_count", "e_nesssr", "e_nsssr", "product_aroring_count", 
        "product_aliring_count", "pains_filter_match_count", "pains_filter_match_name", 
        "transform_id", "r1_hashisy", "r1_smiles", "r2_hashisy", "r2_smiles"
    };

    // Valores por defecto (se mantienen vacíos o con valores numéricos)
    const char *default_values[] = {
        "\"NUEVO_PROD\"", "\"0000000000000000\"", "\"C\"", "C1H1", "0.0", "0", 
        "0", "0", "0.0", "0.0", "0", 
        "0", "0", "0", "0", "0", 
        "0", "0", "", "0", "\"0000000000000000\"", "\"C\"", "\"0000000000000000\"", "\"C\""
    };


    while (client_socket >= 0) { 
        show_menu(); 
        
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n'); 
        
        memset(&request, 0, sizeof(ClientMessage));

        if (choice == 1) {
            // Opción 1: Búsqueda por SMILES (sin cambios)
            printf("Ingrese product_smiles (ej: O(C(=O)C2(CC1=CC=CC=C1)CCC2)CC(OC)C3CCCC3): ");
            if (fgets(current_search_key, sizeof(current_search_key), stdin)) {
                current_search_key[strcspn(current_search_key, "\r\n")] = 0;
            }
            if (current_search_key[0] != '\0') {
                printf("Clave de búsqueda actualizada. Usando clave: %s\n", current_search_key);
                
                request.command = CMD_SEARCH;
                strncpy(request.key_or_record, current_search_key, SEARCH_KEY_MAX - 1);
                send_and_receive(&request, &response);
            }
            
        } else if (choice == 2) {
            // Opción 2: Escribir un nuevo registro 
            printf("\n--- Ingreso de Nuevo Registro ---\n");

            // Solicitar y almacenar los 24 atributos
            for (int i = 0; i < 24; i++) {
                request_attribute(attr_names[i], default_values[i], temp_attrs[i], sizeof(temp_attrs[i]));
            }
            
            // Construir la línea CSV final con comillas triples
            current_record_to_write[0] = '\0'; // Limpiar buffer
            
            for (int i = 0; i < 24; i++) {
                /* 
                Para las 5 columnas de texto (0, 1, 2, 3, 18, 20, 21, 22, 23) y 
                para el campo pains_filter_match_name (índice 18) y los campos hash/smiles
                Debemos añadir """...""" si el valor original no lo tiene, o si lo tiene,
                asegurarnos de que el formato final sea CSV. 
                */

                if (i == 0 || i == 1 || i == 2 || i == 3 || i == 18 || i == 20 || i == 21 || i == 22 || i == 23) {
                    // Campos que deben estar entre comillas triples: (Nombre/Hash/SMILES/Fórmula/Pains)
                    strncat(current_record_to_write, "\"\"", sizeof(current_record_to_write) - strlen(current_record_to_write) - 1);
                    strncat(current_record_to_write, temp_attrs[i], sizeof(current_record_to_write) - strlen(current_record_to_write) - 1);
                    strncat(current_record_to_write, "\"\"", sizeof(current_record_to_write) - strlen(current_record_to_write) - 1);
                } else {
                    // Campos numéricos (o aquellos que van sin comillas triples)
                    strncat(current_record_to_write, temp_attrs[i], sizeof(current_record_to_write) - strlen(current_record_to_write) - 1);
                }
                
                // Añadir coma, excepto después del último campo
                if (i < 23) {
                    strncat(current_record_to_write, ",", sizeof(current_record_to_write) - strlen(current_record_to_write) - 1);
                }
            }

            // Enviar el registro construido al Worker
            if (current_record_to_write[0] != '\0') {
                printf("\nRegistro construido y listo para enviar:\n%s\n", current_record_to_write);
                request.command = CMD_WRITE;
                strncpy(request.key_or_record, current_record_to_write, sizeof(request.key_or_record) - 1);
                send_and_receive(&request, &response);
            }
            
        } else if (choice == 3) {
            // Opción 3: Leer a partir de número de registro 
            int record_num;
            printf("Ingrese el número de registro a buscar (Ej: 1500, contando desde 1, incluyendo encabezado): ");
            if (scanf("%d", &record_num) == 1 && record_num > 0) {
                while (getchar() != '\n');
                
                request.command = CMD_READ_BY_NUM;
                request.record_number = record_num;
                send_and_receive(&request, &response);
            } else {
                while (getchar() != '\n');
                printf("Número de registro inválido.\n");
            }
            
        } else if (choice == 4) {
            // Opción 4: Salir 
            request.command = CMD_EXIT;
            printf("Comando de salida enviado al Worker. Terminando...\n");
            send(client_socket, &request, sizeof(ClientMessage), 0);
            break; 
            
        } else {
            printf("Opción no válida. Intente de nuevo.\n");
        }
    }

    if (client_socket >= 0) {
        close(client_socket); 
    }
}