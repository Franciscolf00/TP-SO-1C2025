#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> // hilos
#include <string.h>
#include <unistd.h> // close()
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h> // inet_ntoa()
#include <sys/socket.h> // socket(), bind(), etc. El struct sockaddr_in se encuentra en esta libreria
#include <stdbool.h>

//defines y estructuras
#define PUERTO 12345
#define MAX_JUGADORES 5

typedef struct {
    int socket;
    char nombre[50];
    int id;
    int puntos;
    bool activo;
}Jugador;

//Variables globales
Jugador listaJugadores[MAX_JUGADORES];
int jugadoresActivos = 0;
int juegoEnCurso = 0;
int id = 0;
pthread_mutex_t mutexId = PTHREAD_MUTEX_INITIALIZER; //mutex para manejar IDs
pthread_mutex_t mutexJugadores = PTHREAD_MUTEX_INITIALIZER; //mutex para manejar jugadores
pthread_cond_t cond_inicio_juego = PTHREAD_COND_INITIALIZER; //sirve para dormir a un hilo o despertarlo dependiendo de si se cumple una condición

//Declaración de funciones
void* manejarJugador (void* pSocketJugador);
void* hiloMaestro(void* arg);
bool crearJugador(int socketJugador);
int buscar_lugar_libre();

int main(int argc, char const *argv[])
{

    int servidor, *nuevoSocket;
    struct sockaddr_in servidorAddr, clienteAddr;
    socklen_t cliente_len = sizeof(clienteAddr);
    bool jugadorCreado;

    servidor = socket(AF_INET, SOCK_STREAM, 0);
    if(servidor < 0)
    {
        perror("Error al crear el socket");
        exit(1);
    }

    servidorAddr.sin_family = AF_INET; //Conexiones ipv4
    servidorAddr.sin_addr.s_addr = INADDR_ANY; //Aceptá conexiones desde cualquier ip
    servidorAddr.sin_port = htons(PUERTO); //Escuchá en este puerto

    if (bind(servidor, (struct sockaddr*)&servidorAddr, sizeof(servidorAddr)) < 0) { // bind asocia un socket a una dirección IP y a un puerto específico que configuré antes.
        perror("Error en bind");
        exit(1);
    }

    if (listen(servidor, MAX_JUGADORES) < 0) { //Pasa el socket del servidor a un estado activo para escuchar nuevas conexiones, (modo escucha)
        perror("Error en listen");             //el segundo parámentro es para indicar el número máximo de conexiones que pueden quedar en cola esperando para entrar al servidor
        exit(1);
    }

    printf("Servidor escuchando en el puerto %d...\n", PUERTO);

    while (1) { //Va a escuchar indefinidamente hasta que se apague, lo descontecta manualmente o después puedo crear una condición para dejar de escuchar
        nuevoSocket = malloc(sizeof(int));
        *nuevoSocket = accept(servidor, (struct sockaddr*)&clienteAddr, &cliente_len); //Acepto conexión de cliente y me guardo el socket creado
        if (*nuevoSocket < 0) {
            perror("Error al aceptar conexión");
            free(nuevoSocket);
            continue;
        }

        if (jugadoresActivos < MAX_JUGADORES) {

            pthread_mutex_lock(&mutexJugadores);
            jugadoresActivos++;
            pthread_mutex_unlock(&mutexJugadores);

            pthread_t hilo;
            pthread_create(&hilo, NULL, manejarJugador, nuevoSocket);
            pthread_detach(hilo);  // No esperamos el hilo, no hago el join

        } else {
            // Imprimir que el servidor está lleno, intentarlo más tarde, etc
        }
    }

    close(servidor);
    return 0;
}



void* manejarJugador (void* pSocketJugador)
{
    int socketJugador = *(int*)pSocketJugador;
    free(pSocketJugador);
    
    char buffer[1024];
    bool jugadorCreado = crearJugador(socketJugador);
    if (!jugadorCreado) {
        close(socketJugador); //cierro socket
        pthread_mutex_lock(&mutexJugadores); //resto jugadores activos
        jugadoresActivos--;
        pthread_mutex_unlock(&mutexJugadores);
        return NULL;
    }

    snprintf(buffer, strlen(buffer), "Bienvenido al servidor.\n");
    send(socketJugador, buffer, sizeof(buffer)-1, 0);

    printf("Jugador conectado.\n");

    // Acá va la lógica del juego

    close(socketJugador);

    pthread_mutex_lock(&mutexJugadores);
    jugadoresActivos--;
    pthread_mutex_unlock(&mutexJugadores);

    return NULL;
}

void* hiloMaestro(void* arg)
{
    return NULL;
}

bool crearJugador (int socketJugador)
{
    char nombre[50];
    char mensaje[128];
    // Pedir nombre
    snprintf(mensaje, sizeof(mensaje), "Ingrese su nombre: ");
    send(socketJugador, mensaje, strlen(mensaje), 0);
    int bytes;
    if ((bytes = recv(socketJugador, nombre, sizeof(nombre), 0)) <= 0) {
        return false;
    }
    nombre[bytes - 1] = '\0'; // remover newline si lo hay

    pthread_mutex_lock(&mutexJugadores);

    int i = buscar_lugar_libre();
    if(i < 0)
    {
        pthread_mutex_unlock(&mutexJugadores);
        return false;
    }

    pthread_mutex_lock(&mutexId); 
    listaJugadores[i].id = id++; //Asigno id e incremento uno
    pthread_mutex_unlock(&mutexId);

    listaJugadores[i].socket = socketJugador;
    strcpy(listaJugadores[i].nombre, nombre);
    listaJugadores[i].puntos = 0;
    listaJugadores[i].activo = 1;
    pthread_mutex_unlock(&mutexJugadores);
    
    return true;
}

int buscar_lugar_libre() {
    for (int i = 0; i < MAX_JUGADORES; i++) {
        if (!listaJugadores[i].activo) 
            return i;
    }
    return -1;  // lista llena
}