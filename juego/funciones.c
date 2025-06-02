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


void enviarMensaje(int socket, const char* mensaje) {
    // Agregar mutex para escritura segura por socket
    static pthread_mutex_t mutexEscritura = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&mutexEscritura);
    write(socket, mensaje, strlen(mensaje));
    pthread_mutex_unlock(&mutexEscritura);
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
    
    //AGREGO PARA PROBAR
     // Resetear turno
    pthread_mutex_lock(&mutexTurno);
    turnoActual = 0;
    pthread_mutex_unlock(&mutexTurno);

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

    pthread_cond_broadcast(&cond_nueva_ronda);
    pthread_mutex_unlock(&mutexJuego);
    printf("Servidor: %s", msg);

    
    //PRUEBO COMENTNADO ESTO
    // IMPORTANTE: Señalar a todos los hilos que hay una nueva ronda
    //pthread_cond_broadcast(&cond_nueva_ronda);
    
   // pthread_mutex_unlock(&mutexJuego);
    
}
  
// Termina el juego, notificando a todos y mostrando puntajes.
void terminarJuego() {
    pthread_mutex_lock(&mutexJuego);      // Bloquea mutex del juego.
    juegoActivo = false;                  // Marca que el juego ya no está activo.
    pthread_mutex_unlock(&mutexJuego);    // Libera mutex.
    //NOtificar terminacion con delay para asegurar el orden
    sleep(1);
    notificarTodos("El juego ha terminado. Gracias por participar.\n"); // Notifica a todos.
    mostrarTablaPuntajes();               // Muestra la tabla de puntajes en consola servidor.
       
    // Despertar a todos los hilos que puedan estar esperando
    pthread_cond_broadcast(&cond_ronda_terminada);
    pthread_cond_broadcast(&cond_nueva_ronda);
    sleep(1);
}

void manejarConexionJugador(int socket) {
    pthread_mutex_lock(&mutexJugadores);

    if (jugadoresConectados >= MAX_JUGADORES || juegoActivo) {
        enviarMensaje(socket, "Sala llena o juego en curso. Quedas en cola.\n");
        agregarACola(socket);
        pthread_mutex_unlock(&mutexJugadores);
        return;
    }

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

    pthread_mutex_lock(&mutexJugadores);
    if (!jugadores[id].activo) {
        pthread_mutex_unlock(&mutexJugadores);
        enviarMensaje(socket, "No se pudo unir a la partida.\n");
        close(socket);
        return;
    }
    pthread_mutex_unlock(&mutexJugadores);

    // BUCLE PRINCIPAL DEL JUEGO
    while (juegoActivo) {
        // Esperar nueva ronda
        pthread_mutex_lock(&mutexJuego);
        while (juegoActivo && rondaTerminada) {
            pthread_cond_wait(&cond_nueva_ronda, &mutexJuego);
        }
        
        if (!juegoActivo) {
            pthread_mutex_unlock(&mutexJuego);
            break;
        }
        pthread_mutex_unlock(&mutexJuego);

        // Dar una oportunidad a cada jugador activo de intentar
        bool jugadorYaIntento = false;
        
        while (juegoActivo && !rondaTerminada && !jugadorYaIntento) {
            // Esperar turno específico del jugador
            pthread_mutex_lock(&mutexTurno);
            while (turnoActual != id && !rondaTerminada && juegoActivo) {
                pthread_mutex_unlock(&mutexTurno);
                usleep(50000); // 50ms
                pthread_mutex_lock(&mutexTurno);
                
                // Verificar si la ronda terminó mientras esperaba
                pthread_mutex_lock(&mutexJuego);
                if (rondaTerminada) {
                    pthread_mutex_unlock(&mutexJuego);
                    pthread_mutex_unlock(&mutexTurno);
                    goto salir_bucle_turnos;
                }
                pthread_mutex_unlock(&mutexJuego);
            }
            
            if (rondaTerminada || !juegoActivo) {
                pthread_mutex_unlock(&mutexTurno);
                break;
            }
            pthread_mutex_unlock(&mutexTurno);

            // Verificar si el jugador sigue activo
            pthread_mutex_lock(&mutexJugadores);
            if (!jugadores[id].activo) {
                pthread_mutex_unlock(&mutexJugadores);
                break;
            }
            
            // Verificar si ya hizo intento
            if (jugadores[id].intentoHecho) {
                pthread_mutex_unlock(&mutexJugadores);
                jugadorYaIntento = true;
                // Pasar turno al siguiente jugador
                pthread_mutex_lock(&mutexTurno);
                turnoActual = (turnoActual + 1) % MAX_JUGADORES;
                pthread_mutex_unlock(&mutexTurno);
                continue;
            }
            pthread_mutex_unlock(&mutexJugadores);

            // Solicitar intento
            enviarMensaje(socket, "Es tu turno. Ingresa un número (1-10): ");

            char buffer[256];
            int leido = read(socket, buffer, sizeof(buffer) - 1);
            if (leido <= 0) {
                // Desconexión
                pthread_mutex_lock(&mutexJugadores);
                jugadores[id].activo = false;
                jugadoresConectados--;
                pthread_mutex_unlock(&mutexJugadores);
                close(socket);
                return;
            }

            buffer[leido] = '\0';
            // Remover salto de línea
            if (buffer[leido-1] == '\n') buffer[leido-1] = '\0';
            
            int intento = atoi(buffer);
            
            if (intento < 1 || intento > 10) {
                enviarMensaje(socket, "Número inválido. Debe estar entre 1 y 10.\n");
                continue;
            }

            // Procesar intento
            pthread_mutex_lock(&mutexJugadores);
            jugadores[id].ultimoIntento = intento;
            jugadores[id].intentoHecho = true;
            pthread_mutex_unlock(&mutexJugadores);

            // Evaluar intento
            evaluarIntento(id);
            jugadorYaIntento = true;

            // Pasar turno al siguiente jugador
            pthread_mutex_lock(&mutexTurno);
            turnoActual = (turnoActual + 1) % MAX_JUGADORES;
            pthread_mutex_unlock(&mutexTurno);
        }
        
        salir_bucle_turnos:;
    }
}

// Función para mostrar resumen de la ronda (opcional, puedes agregarla)
void mostrarResumenRonda() {
    pthread_mutex_lock(&mutexJugadores);
    printf("=== Resumen Ronda %d ===\n", rondaActual);
    printf("Número secreto era: %d\n", numeroSecreto);
    
    char mensajeResumen[500] = "Resumen de la ronda:\n";
    char temp[100];
    
    bool alguienAcerto = false;
    for (int i = 0; i < MAX_JUGADORES; i++) {
        if (jugadores[i].activo) {
            if (jugadores[i].ultimoIntento == numeroSecreto) {
                snprintf(temp, sizeof(temp), "%s acertó\n", jugadores[i].nombre);
                alguienAcerto = true;
            } else {
                snprintf(temp, sizeof(temp), "%s intentó %d\n", 
                        jugadores[i].nombre, jugadores[i].ultimoIntento);
            }
            strcat(mensajeResumen, temp);
            printf("%s", temp);
        }
    }
    
    if (!alguienAcerto) {
        strcat(mensajeResumen, "Nadie acertó esta ronda.\n");
        printf("Nadie acertó esta ronda.\n");
    }
    
    printf("=====================\n");
    pthread_mutex_unlock(&mutexJugadores);
    
    // Enviar resumen a todos los jugadores
    notificarTodos(mensajeResumen);
}


// Función  para verificar si todos los jugadores activos intentaron
bool todosIntentaronOTimeout() {
    pthread_mutex_lock(&mutexJugadores);
    bool todosIntentaron = true;
    int jugadoresActivos = 0;
    
    for (int i = 0; i < MAX_JUGADORES; i++) {
        if (jugadores[i].activo) {
            jugadoresActivos++;
            if (!jugadores[i].intentoHecho) {
                todosIntentaron = false;
            }
        }
    }
    
    pthread_mutex_unlock(&mutexJugadores);
    
    // Si no hay jugadores activos, considerar que "todos" intentaron
    if (jugadoresActivos == 0) {
        todosIntentaron = true;
    }
    
    return todosIntentaron;
}


// Función  para evaluar un intento individual
void evaluarIntento(int jugadorId) {
    pthread_mutex_lock(&mutexJugadores);
    if (!jugadores[jugadorId].activo || !jugadores[jugadorId].intentoHecho) {
        pthread_mutex_unlock(&mutexJugadores);
        return;
    }
    
    int intento = jugadores[jugadorId].ultimoIntento;
    bool acerto = (intento == numeroSecreto);
    
    // Enviar respuesta individual inmediatamente
    char respuesta[100];
    if (acerto) {
        snprintf(respuesta, sizeof(respuesta), "¡Correcto! El número era %d. ¡Ganaste!\n", numeroSecreto);
        jugadores[jugadorId].puntos++;
        
        // Notificar a todos sobre este ganador (sin terminar la ronda aún)
        char msgGanador[100];
        snprintf(msgGanador, sizeof(msgGanador), "¡%s acertó el número!\n", jugadores[jugadorId].nombre);
        pthread_mutex_unlock(&mutexJugadores); // Desbloquear antes de notificar
        
        enviarMensaje(jugadores[jugadorId].socket, respuesta);
        notificarTodos(msgGanador);
        
        pthread_mutex_lock(&mutexJugadores); // Volver a bloquear
    } else if (intento < numeroSecreto) {
        snprintf(respuesta, sizeof(respuesta), "Tu número %d es menor. El número secreto es mayor.\n", intento);
        enviarMensaje(jugadores[jugadorId].socket, respuesta);
    } else {
        snprintf(respuesta, sizeof(respuesta), "Tu número %d es mayor. El número secreto es menor.\n", intento);
        enviarMensaje(jugadores[jugadorId].socket, respuesta);
    }
    
    pthread_mutex_unlock(&mutexJugadores);
    
    // IMPORTANTE: No terminar la ronda aca, solo verificar si todos intentaron
    if (todosIntentaronOTimeout()) {
        pthread_mutex_lock(&mutexJuego);
        rondaTerminada = true;
        pthread_cond_signal(&cond_ronda_terminada);
        pthread_mutex_unlock(&mutexJuego);
    }
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

