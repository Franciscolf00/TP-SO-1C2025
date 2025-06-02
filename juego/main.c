#include <stdio.h>              
#include <stdlib.h>         
#include <string.h>           
#include <unistd.h>             // Funciones POSIX como close(), read(), write()
#include <pthread.h>            // Biblioteca para manejo de hilos (pthreads)
#include <netinet/in.h>         // Estructuras para sockets (sockaddr_in, etc.)
#include <sys/socket.h>         // Funciones y constantes para sockets

#include "variablesglobal.h"    
#include "funciones.h"          

#define PUERTO 12345            // Puerto en el que el servidor escuchará conexiones

//main = servidor

// Prototipo de función para manejar a cada cliente
void *manejadorCliente(void *arg);

int main() {
    int servidor, nuevoSocket;                         // Descriptor del socket del servidor y del nuevo cliente
    struct sockaddr_in servidorAddr, clienteAddr;      // Estructuras con dirección del servidor y cliente
    socklen_t clienteLen = sizeof(clienteAddr);        // Longitud de la estructura cliente
    pthread_t hiloCliente;                             // ID del hilo para manejar clientes
    int opt = 1;                                        // Opción usada para setsockopt()
    srand(time(NULL));

    // Crear socket TCP (AF_INET, SOCK_STREAM)
    servidor = socket(AF_INET, SOCK_STREAM, 0);
    if (servidor == -1) {
        perror("Error al crear socket");               // Si falla, muestra error y termina
        exit(EXIT_FAILURE);
    }

    // Permitir reutilizar dirección local (evita "Address already in use")
    if (setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(servidor);
        exit(EXIT_FAILURE);
    }

    // Configurar dirección del servidor
    servidorAddr.sin_family = AF_INET;                 // IPv4
    servidorAddr.sin_addr.s_addr = INADDR_ANY;         // Acepta conexiones desde cualquier IP local
    servidorAddr.sin_port = htons(PUERTO);             // Puerto en orden de red
    memset(servidorAddr.sin_zero, 0, sizeof servidorAddr.sin_zero); // Relleno en 0

    // Asociar socket con dirección y puerto (bind)
    if (bind(servidor, (struct sockaddr *)&servidorAddr, sizeof(servidorAddr)) < 0) {
        perror("Error en bind");
        close(servidor);
        exit(EXIT_FAILURE);
    }

    // Poner el socket en modo escucha (escuchar conexiones entrantes)
    if (listen(servidor, MAX_JUGADORES) < 0) {
        perror("Error en listen");
        close(servidor);
        exit(EXIT_FAILURE);
    }

    printf("Servidor escuchando en puerto %d...\n", PUERTO);

    // Crear un hilo separado que maneja el flujo del juego (inicio, rondas, etc.)
    pthread_t hiloJuego;
    //pthread_create(&hiloJuego, NULL, (void *)iniciarJuego, NULL);
    pthread_create(&hiloJuego, NULL, (void*(*)(void*))iniciarJuego, NULL);
    pthread_detach(hiloJuego);  // Se separa para que se ejecute en segundo plano

    // Bucle principal: acepta conexiones entrantes de nuevos jugadores
    while (1) {
        nuevoSocket = accept(servidor, (struct sockaddr *)&clienteAddr, &clienteLen);
        if (nuevoSocket < 0) {
            perror("Error en accept");  // Si hay error en accept, lo informa y sigue
            continue;
        }

        // Crear un hilo nuevo para manejar al cliente conectado
        pthread_t hiloCliente;
        pthread_create(&hiloCliente, NULL, manejadorCliente, (void *)&nuevoSocket);
        pthread_detach(hiloCliente);  // Se separa para que no sea necesario hacer join
    }

    close(servidor);  // Cierra el socket del servidor (nunca se alcanza por el while(1))
    return 0;
}

// Función que maneja la conexión con un cliente individual
void *manejadorCliente(void *arg) {
    int socketCliente = *((int *)arg);                 // Extrae el socket del cliente recibido por argumento
    manejarConexionJugador(socketCliente);             // Llama a la función que gestiona la lógica del jugador
    return NULL;
}
