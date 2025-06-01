#ifndef VARIABLESGLOBAL_H
#define VARIABLESGLOBAL_H

#include <pthread.h>   // Biblioteca para usar hilos (pthread), mutex y variables de condición
#include <stdbool.h>   // Permite usar el tipo de dato booleano (bool, true, false)

// Cantidad máxima de jugadores activos que pueden participar al mismo tiempo
#define MAX_JUGADORES 5

// Cantidad máxima de jugadores en la cola de espera
#define MAX_COLA_ESPERA 10

// Estructura que representa a cada jugador
typedef struct {
    int socket;               // Descriptor del socket del jugador (conexión con el cliente)
    char nombre[50];          // Nombre del jugador (enviado por el cliente)
    int puntos;               // Puntaje acumulado en el juego
    bool activo;              // Indica si el jugador está participando activamente en la partida
    bool quiereSeguir;        // Si el jugador desea seguir participando tras finalizar una ronda
    int ultimoIntento;        // Último número que el jugador envió para adivinar
    bool intentoHecho;        // Marca si el jugador ya hizo su intento en la ronda actual
} Jugador;

// Arreglo de jugadores activos en el juego (máximo MAX_JUGADORES)
extern Jugador jugadores[MAX_JUGADORES];

// Cantidad total de jugadores actualmente conectados (activos + en cola)
extern int jugadoresConectados;

// Cantidad de jugadores que indicaron que están listos para jugar
extern int jugadoresListos;

// Cantidad de jugadores que están jugando en la ronda actual
extern int jugadoresJugando;

// Cola de espera para jugadores que no pueden ingresar porque ya hay MAX_JUGADORES activos
extern int colaEspera[MAX_COLA_ESPERA]; // Cola circular de sockets o IDs de jugadores
extern int frenteCola;                 // Índice del primer elemento en la cola
extern int finalCola;                  // Índice del último elemento en la cola

// Variables que controlan el estado general del juego
extern int numeroSecreto;      // Número secreto que los jugadores deben adivinar (1-10)
extern int rondaActual;        // Número de la ronda actual
extern bool juegoActivo;       // Indica si una ronda está activa
extern bool rondaTerminada;    // Indica si todos los jugadores ya jugaron y se terminó la ronda

// Mutexes para proteger secciones críticas del código cuando hay acceso concurrente
extern pthread_mutex_t mutexJugadores; // Protege el acceso al arreglo de jugadores
extern pthread_mutex_t mutexCola;      // Protege el acceso a la cola de espera
extern pthread_mutex_t mutexJuego;     // Protege variables del estado del juego

// Variables de condición para coordinar el flujo de ejecución entre los hilos
extern pthread_cond_t cond_inicio_juego;     // Señaliza que ya hay suficientes jugadores para comenzar
extern pthread_cond_t cond_turno;            // Coordina el turno de cada jugador
extern pthread_cond_t cond_ronda_terminada;  // Señaliza que terminó la ronda actual
extern pthread_cond_t cond_todosIntentos;    // Señaliza que todos los jugadores ya hicieron su intento

#endif 
