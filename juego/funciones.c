#include <stdio.h>        // Incluye funciones estándar para entrada/salida, como printf.
#include <stdlib.h>       // Incluye funciones estándar como malloc, rand, exit.
#include <string.h>       // Funciones para manejo de cadenas, como strlen, snprintf.
#include <unistd.h>       // Funciones POSIX, como write, read, close, usleep.
#include <time.h>         // Funciones para manejo de tiempo, como time, srand.
#include "variablesglobal.h" // Archivo propio que declara variables globales (ej. mutex, jugadores).
#include "funciones.h"    // Archivo propio con declaraciones de funciones utilizadas aquí.


// Función que controla el ciclo general del juego: iniciar rondas, esperar fin, etc.
//ESTA FUNCIONA
void iniciarJuego() {
    pthread_mutex_lock(&mutexJuego);
    while(!juegoActivo)
    {
        //Espero a que juegoActivo sea true
        pthread_cond_wait(&cond_juego_activo, &mutexJuego);
    }
    pthread_mutex_unlock(&mutexJuego);    

    int rondasCompletadas = 0;

    while (juegoActivo && rondasCompletadas < 3) {
        iniciarRonda();  // Genera número secreto y resetea intentos.

        // Espera hasta que la ronda termine (cuando alguien acierta).
        pthread_mutex_lock(&mutexJuego);
        while (!rondaTerminada) {
            pthread_cond_wait(&cond_ronda_terminada, &mutexJuego);
        }
        pthread_mutex_unlock(&mutexJuego);
        rondasCompletadas++;
        sleep(2);  // Espera 2 segundos antes de iniciar la próxima ronda.
    }

    terminarJuego();  // Finaliza el juego, notifica y muestra puntajes.
}


// Envía un mensaje (string) al socket indicado.
void enviarMensaje(int socket, const char* mensaje) {
    write(socket, mensaje, strlen(mensaje)); // Escribe el mensaje en el socket, usando la longitud del string.
}

// Envía un mensaje a todos los jugadores activos.
void notificarTodos(const char* mensaje) {
    pthread_mutex_lock(&mutexJugadores);  // Bloquea el mutex para acceder seguro al arreglo de jugadores.
    for (int i = 0; i < MAX_JUGADORES; i++) {
        if (jugadores[i].activo) {         // Si el jugador está activo (conectado y en juego).
            enviarMensaje(jugadores[i].socket, mensaje); // Envía el mensaje a ese jugador.
        }
    }
    pthread_mutex_unlock(&mutexJugadores); // Libera el mutex.
}

// Imprime la tabla de puntajes de todos los jugadores activos en el servidor.
void mostrarTablaPuntajes() {
    pthread_mutex_lock(&mutexJugadores);   // Bloquea acceso concurrente a jugadores.
    printf("===== Tabla de Puntajes =====\n");
    for (int i = 0; i < MAX_JUGADORES; i++) {
        if (jugadores[i].activo) {         // Solo jugadores activos.
            printf("%s - %d puntos\n", jugadores[i].nombre, jugadores[i].puntos); // Muestra nombre y puntos.
        }
    }
    printf("=============================\n");
    pthread_mutex_unlock(&mutexJugadores); // Desbloquea mutex.
}

void iniciarRonda() {
    pthread_mutex_lock(&mutexJuego);
    rondaActual++;
    numeroSecreto = rand() % 10 + 1;
    printf("Numero secreto: %d\n",numeroSecreto);
    rondaTerminada = false;  // Importante: marcar como NO terminada al inicio
    
    // Resetear intentos de jugadores
    pthread_mutex_lock(&mutexJugadores);
    for (int i = 0; i < MAX_JUGADORES; i++) {
        if (jugadores[i].activo) {
            jugadores[i].intentoHecho = false;
            jugadores[i].ultimoIntento = -1;
        }
    }
    pthread_mutex_unlock(&mutexJugadores);

    // Notificar nueva ronda
    char msg[100];
    snprintf(msg, sizeof(msg), "Nueva ronda #%d iniciada. ¡Adivina el número entre 1 y 10!\n", rondaActual);
    notificarTodos(msg);
    printf("Servidor: %s", msg);
    
    // IMPORTANTE: Señalar a todos los hilos que hay una nueva ronda
    pthread_cond_broadcast(&cond_nueva_ronda);
    
    pthread_mutex_unlock(&mutexJuego);
}
  
// Termina el juego, notificando a todos y mostrando puntajes.
void terminarJuego() {
    pthread_mutex_lock(&mutexJuego);      // Bloquea mutex del juego.
    juegoActivo = false;                  // Marca que el juego ya no está activo.
    notificarTodos("El juego ha terminado. Gracias por participar.\n"); // Notifica a todos.
    mostrarTablaPuntajes();               // Muestra la tabla de puntajes en consola servidor.
    pthread_mutex_unlock(&mutexJuego);   // Libera mutex.
}


//Esta ya funciona, mas o menos

void manejarConexionJugador(int socket) {
    pthread_mutex_lock(&mutexJugadores);

    // Si la sala está llena o el juego ya está activo, pone al jugador en cola.
    if (jugadoresConectados >= MAX_JUGADORES || juegoActivo) {
        enviarMensaje(socket, "Sala llena o juego en curso. Quedas en cola.\n");
        agregarACola(socket);
        pthread_mutex_unlock(&mutexJugadores);
        return;
    }

    // Asigna id al nuevo jugador
    int id = jugadoresConectados;
    jugadores[id].socket = socket;
    jugadores[id].puntos = 0;
    jugadores[id].activo = true;
    jugadores[id].quiereSeguir = true;
    jugadores[id].intentoHecho = false;
    jugadores[id].ultimoIntento = -1;
    snprintf(jugadores[id].nombre, sizeof(jugadores[id].nombre), "Jugador%d", id+1);

    jugadoresConectados++;
    pthread_mutex_unlock(&mutexJugadores);

    enviarMensaje(socket, "Bienvenido. Esperando inicio de juego...\n");
    esperarJugadores();

    // Verifica que el jugador siga activo
    pthread_mutex_lock(&mutexJugadores);
    if (!jugadores[id].activo) {
        pthread_mutex_unlock(&mutexJugadores);
        enviarMensaje(socket, "No se pudo unir a la partida.\n");
        close(socket);
        return;
    }
    pthread_mutex_unlock(&mutexJugadores);

    // BUCLE PRINCIPAL POR RONDA - cada jugador espera su turno en cada ronda
    while (juegoActivo) {
        // ESPERAR A QUE INICIE UNA NUEVA RONDA
        pthread_mutex_lock(&mutexJuego);
        while (juegoActivo && rondaTerminada) {
            // Espera a que la ronda actual termine y comience una nueva
            pthread_cond_wait(&cond_nueva_ronda, &mutexJuego);
        }
        
        if (!juegoActivo) {
            pthread_mutex_unlock(&mutexJuego);
            break;
        }
        pthread_mutex_unlock(&mutexJuego);

        // BUCLE DE TURNOS DENTRO DE LA RONDA
        while (juegoActivo) {
            pthread_mutex_lock(&mutexJuego);
            if (rondaTerminada) {
                pthread_mutex_unlock(&mutexJuego);
                break; // Sale del bucle de turnos para esperar nueva ronda
            }
            pthread_mutex_unlock(&mutexJuego);

            // Verificar si ya hizo intento en este turno
            pthread_mutex_lock(&mutexJugadores);
            if (jugadores[id].intentoHecho) {
                pthread_mutex_unlock(&mutexJugadores);
                // Ya hizo su intento, espera a que todos terminen
                pthread_mutex_lock(&mutexJuego);
                while (!rondaTerminada && !todosIntentaronOTimeout()) {
                    pthread_mutex_unlock(&mutexJuego);
                    usleep(100000); // Espera 100ms
                    pthread_mutex_lock(&mutexJuego);
                }
                pthread_mutex_unlock(&mutexJuego);
                continue;
            }
            pthread_mutex_unlock(&mutexJugadores);

            // Pedir intento al jugador
            enviarMensaje(socket, "Es tu turno para enviar un número (1-10):\n");

            char buffer[256];
            int leido = read(socket, buffer, sizeof(buffer) - 1);
            if (leido <= 0) {
                // Desconexión del jugador
                pthread_mutex_lock(&mutexJugadores);
                jugadores[id].activo = false;
                jugadoresConectados--;
                pthread_mutex_unlock(&mutexJugadores);
                close(socket);
                return;
            }

            buffer[leido] = '\0';
            int intento = atoi(buffer);
            
            if (intento < 1 || intento > 10) {
                enviarMensaje(socket, "Número inválido, debe estar entre 1 y 10.\n");
                continue;
            }

            // Guardar intento
            pthread_mutex_lock(&mutexJugadores);
            jugadores[id].ultimoIntento = intento;
            jugadores[id].intentoHecho = true;
            pthread_mutex_unlock(&mutexJugadores);

            // Esperar a que todos hagan intento o timeout
            esperarTodosIntentos();

            // Evaluar intentos (solo un hilo debe hacer esto)
            evaluarIntentos();
        }
    }
    
    // Preguntar si quiere seguir jugando
    enviarMensaje(socket, "¿Querés seguir jugando? (s/n):\n");
    char respuesta[10];
    int r = read(socket, respuesta, sizeof(respuesta)-1);
    if (r > 0) {
        respuesta[r] = '\0';
        if (respuesta[0] == 'n' || respuesta[0] == 'N') {
            pthread_mutex_lock(&mutexJugadores);
            jugadores[id].activo = false;
            jugadoresConectados--;
            pthread_mutex_unlock(&mutexJugadores);
            enviarMensaje(socket, "Gracias por jugar!\n");
            close(socket);

            // Sacar jugador de cola si hay
            pthread_mutex_lock(&mutexCola);
            if (frenteCola != finalCola) {
                int s = colaEspera[frenteCola];
                frenteCola = (frenteCola + 1) % MAX_COLA_ESPERA;
                pthread_mutex_unlock(&mutexCola);
                manejarConexionJugador(s);
                return;
            }
            pthread_mutex_unlock(&mutexCola);
        }
    }
    close(socket);
}



bool todosIntentaronOTimeout() {
    static time_t inicioEspera = 0;
    if (inicioEspera == 0) inicioEspera = time(NULL);
    
    pthread_mutex_lock(&mutexJugadores);
    bool todosIntentaron = true;
    for (int i = 0; i < MAX_JUGADORES; i++) {
        if (jugadores[i].activo && !jugadores[i].intentoHecho) {
            todosIntentaron = false;
            break;
        }
    }
    pthread_mutex_unlock(&mutexJugadores);
    
    bool timeout = (time(NULL) - inicioEspera >= 10);
    
    if (todosIntentaron || timeout) {
        inicioEspera = 0; // Reset para próxima vez
        return true;
    }
    return false;
}

void esperarTodosIntentos() {
    time_t inicio = time(NULL);
    while (!todosIntentaronOTimeout() && time(NULL) - inicio < 10) {
        usleep(100000); // 100ms
    }
}

void evaluarIntentos() {
    static pthread_mutex_t mutexEvaluacion = PTHREAD_MUTEX_INITIALIZER;
    
    // Solo un hilo debe evaluar
    if (pthread_mutex_trylock(&mutexEvaluacion) != 0) {
        return; // Otro hilo ya está evaluando
    }
    
    pthread_mutex_lock(&mutexJugadores);
    bool alguienAcerto = false;
    int ganador = -1;
    
    // Evaluar intentos
    for (int i = 0; i < MAX_JUGADORES; i++) {
        if (!jugadores[i].activo || !jugadores[i].intentoHecho) continue;
        if (jugadores[i].ultimoIntento == numeroSecreto) {
            alguienAcerto = true;
            ganador = i;
            jugadores[i].puntos++;
        }
    }
    
    // Enviar respuestas individuales
    for (int i = 0; i < MAX_JUGADORES; i++) {
        if (!jugadores[i].activo || !jugadores[i].intentoHecho) continue;
        char resp[50];
        int val = jugadores[i].ultimoIntento;
        if (val == numeroSecreto) {
            snprintf(resp, sizeof(resp), "¡Acertaste!\n");
        } else if (val < numeroSecreto) {
            snprintf(resp, sizeof(resp), "Mayor.\n");
        } else {
            snprintf(resp, sizeof(resp), "Menor.\n");
        }
        enviarMensaje(jugadores[i].socket, resp);
    }
    pthread_mutex_unlock(&mutexJugadores);
    
    if (alguienAcerto) {
        char msg[100];
        snprintf(msg, sizeof(msg), "¡Jugador %d ganó la ronda %d!\n", ganador + 1, rondaActual);
        notificarTodos(msg);
        
        pthread_mutex_lock(&mutexJuego);
        rondaTerminada = true;
        pthread_cond_signal(&cond_ronda_terminada);
        pthread_mutex_unlock(&mutexJuego);
    } else {
        // Resetear intentos para siguiente turno
        pthread_mutex_lock(&mutexJugadores);
        for (int i = 0; i < MAX_JUGADORES; i++) {
            if (jugadores[i].activo) jugadores[i].intentoHecho = false;
        }
        pthread_mutex_unlock(&mutexJugadores);
    }
    
    pthread_mutex_unlock(&mutexEvaluacion);
}



// Agrega un socket a la cola de espera si la sala está llena.
void agregarACola(int socketCliente) {
    pthread_mutex_lock(&mutexCola);           // Bloquea mutex de cola.
    int siguiente = (finalCola + 1) % MAX_COLA_ESPERA; // Calcula posición siguiente.
    if (siguiente != frenteCola) {            // Si la cola no está llena.
        colaEspera[finalCola] = socketCliente; // Guarda socket en cola.
        finalCola = siguiente;                 // Avanza puntero final.
    } else {
        enviarMensaje(socketCliente, "Cola de espera llena, intente más tarde.\n");
        close(socketCliente);                   // Cierra socket si cola llena.
    }
    pthread_mutex_unlock(&mutexCola);         // Libera mutex cola.
}

// Espera a que haya al menos 2 jugadores y hasta 2 minutos o máximo 5 jugadores.
void esperarJugadores() {
    pthread_mutex_lock(&mutexJuego);
    time_t start = time(NULL);

    while (1) {
        pthread_mutex_lock(&mutexJugadores);
        int cnt = jugadoresConectados;
        pthread_mutex_unlock(&mutexJugadores);

        if (cnt >= 2) {
            if (time(NULL) - start >= 120 || cnt == MAX_JUGADORES) break;
        }
        if (time(NULL) - start >= 300) { // 5 minutos sin mínimo jugadores, termina programa.
            juegoActivo = false;
            pthread_mutex_unlock(&mutexJuego);
            exit(0);
        }
        pthread_mutex_unlock(&mutexJuego);
        usleep(500000); // Espera 0.5 segundos antes de seguir chequeando.
        pthread_mutex_lock(&mutexJuego);
    }
    juegoActivo = true;
    pthread_cond_signal(&cond_juego_activo);
    pthread_cond_broadcast(&cond_inicio_juego); // Señala a todos que el juego inicia.
    pthread_mutex_unlock(&mutexJuego);
}

// Señala el inicio del juego si hay al menos 2 jugadores.
void verificarInicioJuego() {
    pthread_mutex_lock(&mutexJugadores);
    if (jugadoresConectados >= 2) {
        pthread_mutex_unlock(&mutexJugadores);
        pthread_cond_signal(&cond_inicio_juego); // Señala a hilos esperando iniciar.
    } else {
        pthread_mutex_unlock(&mutexJugadores);
    }
}

