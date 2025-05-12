#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> // pthreads
#include <string.h>
#include <unistd.h> // close()
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h> // inet_ntoa()
#include <sys/socket.h> // socket(), bind(), etc.



#define PUERTO 12345
#define MAX_CLIENTES 5

int clientesActivos = 0;
pthread_mutex_t mutexClientes = PTHREAD_MUTEX_INITIALIZER;

void* manejarCliente (void* pSocketCliente);

int main(int argc, char const *argv[])
{
    int servidor, *nuevoSocket;
    struct sockaddr_in servidorAddr, clienteAddr;
    socklen_t cliente_len = sizeof(clienteAddr);

    servidor = socket(AF_INET, SOCK_STREAM, 0);
    if(servidor < 0)
    {
        perror("Error al crear el socket");
        exit(1);
    }

    servidor_addr.sin_family = AF_INET;
    servidor_addr.sin_addr.s_addr = INADDR_ANY;
    servidor_addr.sin_port = htons(PUERTO);

    if (bind(servidor, (struct sockaddr*)&servidor_addr, sizeof(servidor_addr)) < 0) {
        perror("Error en bind");
        exit(1);
    }

    if (listen(servidor, MAX_CLIENTES) < 0) {
        perror("Error en listen");
        exit(1);
    }

    printf("Servidor escuchando en el puerto %d...\n", PUERTO);

    while (1) {
        if (clientes_activos < MAX_CLIENTES) {
            nuevo_socket = malloc(sizeof(int));
            *nuevo_socket = accept(servidor, (struct sockaddr*)&cliente_addr, &cliente_len);
            if (*nuevo_socket < 0) {
                perror("Error al aceptar conexión");
                free(nuevo_socket);
                continue;
            }

            pthread_mutex_lock(&mutex_clientes);
            clientes_activos++;
            pthread_mutex_unlock(&mutex_clientes);

            pthread_t hilo;
            pthread_create(&hilo, NULL, manejar_cliente, nuevo_socket);
            pthread_detach(hilo);  // No esperamos el hilo
        } else {
            // Opcional: podrías imprimir que está lleno
        }
    }

    close(servidor);
    return 0;
}

void* manejarCliente (void* pSocketCliente)
{
    int socketCliente = (int*)pSocketCliente;
    free(pSocketCliente);

    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "Bienvenido al servidor.\n");
    send(socketCliente, buffer);

    printf("Jugador conectado.\n");

    // Acá va la lógica del juego

    close(socketCliente);

    pthread_mutex_lock(&mutexClientes);
    clientesActivos--;
    pthread_mutex_unlock(&mutexClientes);

    return NULL;
}