#include "variablesglobal.h"  // Incluye las declaraciones de variables, estructuras y constantes globales

// Arreglo de jugadores activos en el juego
Jugador jugadores[MAX_JUGADORES];

// Contador de jugadores conectados al servidor (activos o en espera)
int jugadoresConectados = 0;

// Contador de jugadores que están listos para comenzar una partida
int jugadoresListos = 0;

// Contador de jugadores que están participando activamente en la ronda
int jugadoresJugando = 0;

// Cola circular para guardar sockets de jugadores que esperan ingresar al juego
int colaEspera[MAX_COLA_ESPERA];

// Índice del primer elemento de la cola de espera
int frenteCola = 0;

// Índice donde se insertará el próximo jugador en la cola de espera
int finalCola = 0;

// Número secreto que deben adivinar los jugadores (se genera aleatoriamente)
int numeroSecreto;

// Número de la ronda actual
int rondaActual = 0;

// Indica si el juego está activo (true) o no (false)
bool juegoActivo = false;

// Indica si la ronda actual ha terminado
bool rondaTerminada = false;

// Mutex para proteger el acceso concurrente a la estructura de jugadores
pthread_mutex_t mutexJugadores = PTHREAD_MUTEX_INITIALIZER;

// Mutex para proteger el acceso concurrente a la cola de espera
pthread_mutex_t mutexCola = PTHREAD_MUTEX_INITIALIZER;

// Mutex para proteger el acceso a las variables del juego
pthread_mutex_t mutexJuego = PTHREAD_MUTEX_INITIALIZER;

// Variable de condición que permite comenzar el juego cuando haya suficientes jugadores
pthread_cond_t cond_inicio_juego = PTHREAD_COND_INITIALIZER;

// Variable de condición que gestiona el turno de los jugadores (quién puede jugar)
pthread_cond_t cond_turno = PTHREAD_COND_INITIALIZER;

// Variable de condición que indica que la ronda ha terminado
pthread_cond_t cond_ronda_terminada = PTHREAD_COND_INITIALIZER;

// Variable de condición que indica que todos los jugadores realizaron su intento
pthread_cond_t cond_todosIntentos = PTHREAD_COND_INITIALIZER;

pthread_cond_t cond_juego_activo = PTHREAD_COND_INITIALIZER;

pthread_cond_t cond_nueva_ronda = PTHREAD_COND_INITIALIZER;
