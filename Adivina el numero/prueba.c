#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PUERTO 8080
#define MAX_CLIENTES 1

pthread_mutex_t mutex_clientes = PTHREAD_MUTEX_INITIALIZER;
int clientes_activos = 0;

void* manejarCliente(void* socket_desc);

int main() {
    int servidor, *nuevo_socket;
    struct sockaddr_in servidorAddr, clienteAddr;
    socklen_t cliente_len = sizeof(clienteAddr);

    servidor = socket(AF_INET, SOCK_STREAM, 0);
    if (servidor < 0) {
        perror("Error al crear socket");
        return 1;
    }

    servidorAddr.sin_family = AF_INET;
    servidorAddr.sin_addr.s_addr = INADDR_ANY;
    servidorAddr.sin_port = htons(PUERTO);

    if (bind(servidor, (struct sockaddr*)&servidorAddr, sizeof(servidorAddr)) < 0) {
        perror("Error en bind");
        return 1;
    }

    listen(servidor, MAX_CLIENTES);
    printf("Servidor escuchando en puerto %d...\n", PUERTO);

    /* while (1) {
        if (clientes_activos < MAX_CLIENTES) {
            nuevo_socket = malloc(sizeof(int));
            *nuevo_socket = accept(servidor, (struct sockaddr*)&clienteAddr, &cliente_len);
            if (*nuevo_socket < 0) {
                perror("Error al aceptar conexión");
                free(nuevo_socket);
                continue;
            }

            pthread_mutex_lock(&mutex_clientes);
            clientes_activos++;
            pthread_mutex_unlock(&mutex_clientes);

            pthread_t hilo;
            pthread_create(&hilo, NULL, manejarCliente, nuevo_socket);
            pthread_detach(hilo);  // Liberamos automáticamente los recursos del hilo
        } else {
            printf("Máximo de clientes alcanzado\n");
            sleep(1);  // Pequeña espera para evitar consumir CPU
        }
    } */
    while (1) {
        nuevo_socket = malloc(sizeof(int));
        *nuevo_socket = accept(servidor, (struct sockaddr*)&clienteAddr, &cliente_len);
        if (*nuevo_socket < 0) {
            perror("Error al aceptar conexión");
            free(nuevo_socket);
            continue;
        }

        pthread_mutex_lock(&mutex_clientes);
        if (clientes_activos < MAX_CLIENTES) {
            clientes_activos++;
            pthread_mutex_unlock(&mutex_clientes);

            pthread_t hilo;
            pthread_create(&hilo, NULL, manejarCliente, nuevo_socket);
            pthread_detach(hilo);
        } else {
            pthread_mutex_unlock(&mutex_clientes);
            char* msg = "Servidor lleno, intente más tarde\n";
            send(*nuevo_socket, msg, strlen(msg), 0);
            close(*nuevo_socket);
            free(nuevo_socket);
        }
    }

    close(servidor);
    return 0;
}

void* manejarCliente(void* socket_desc) {
    int socket = *(int*)socket_desc;
    free(socket_desc);
    char buffer[1024];
    int leido;

    while ((leido = recv(socket, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[leido] = '\0';
        printf("Mensaje recibido: %s\n", buffer);
    }

    close(socket);
    pthread_mutex_lock(&mutex_clientes);
    clientes_activos--;
    pthread_mutex_unlock(&mutex_clientes);
    printf("Cliente desconectado\n");
    return NULL;
}