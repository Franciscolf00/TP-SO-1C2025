#include <stdio.h>        // Incluye funciones estándar para entrada/salida, como printf.
#include <stdlib.h>       // Incluye funciones estándar como malloc, rand, exit.
#include <string.h>       // Funciones para manejo de cadenas, como strlen, snprintf.
#include <unistd.h>       // Funciones POSIX, como write, read, close, usleep.
#include <time.h>         // Funciones para manejo de tiempo, como time, srand.
#include "variablesglobal.h" // Archivo propio que declara variables globales (ej. mutex, jugadores).
#include "funciones.h"    // Archivo propio con declaraciones de funciones utilizadas aquí.


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

// Inicializa una nueva ronda del juego.
void iniciarRonda() {
    pthread_mutex_lock(&mutexJuego);       // Bloquea el mutex del juego para sincronizar cambios.
    rondaActual++;                         // Incrementa el número de ronda.
    numeroSecreto = rand() % 10 + 1;      // Genera un número secreto aleatorio entre 1 y 10.
    rondaTerminada = false;                // Marca la ronda como no terminada.
    //printf("%d \n",numeroSecreto );
    // Resetea los intentos de los jugadores.
    pthread_mutex_lock(&mutexJugadores);
    for (int i = 0; i < MAX_JUGADORES; i++) {
        if (jugadores[i].activo) {
            jugadores[i].intentoHecho = false;  // Marca que no hicieron intento aún.
            jugadores[i].ultimoIntento = -1;    // Reinicia último intento.
        }
    }
    pthread_mutex_unlock(&mutexJugadores);

    // Construye mensaje de inicio de ronda e informa a todos los jugadores.
    char msg[100];
    //char msg2[100];
    snprintf(msg, sizeof(msg), "Nueva ronda #%d iniciada. ¡Adivina el número entre 1 y 10!\n", rondaActual);
    //snprintf(msg2, sizeof(msg2), "numero adivnar", numeroSecreto); 
    notificarTodos(msg);                     // Envía el mensaje a todos.
    //notificarTodos(msg2);  
    printf("Servidor: %s", msg);             // También imprime en consola servidor.
    pthread_mutex_unlock(&mutexJuego);       // Libera mutex del juego.
}

// Termina el juego, notificando a todos y mostrando puntajes.
void terminarJuego() {
    pthread_mutex_lock(&mutexJuego);      // Bloquea mutex del juego.
    juegoActivo = false;                  // Marca que el juego ya no está activo.
    notificarTodos("El juego ha terminado. Gracias por participar.\n"); // Notifica a todos.
    mostrarTablaPuntajes();               // Muestra la tabla de puntajes en consola servidor.
    pthread_mutex_unlock(&mutexJuego);   // Libera mutex.
}

// Maneja la conexión de un jugador que se conecta por socket.
/*void manejarConexionJugador(int socket) {
    pthread_mutex_lock(&mutexJugadores); // Bloquea acceso a la lista de jugadores.

    // Si la sala está llena o el juego ya está activo, pone al jugador en cola.
    if (jugadoresConectados >= MAX_JUGADORES || juegoActivo) {
        enviarMensaje(socket, "Sala llena o juego en curso. Quedas en cola.\n");
        agregarACola(socket);             // Agrega el socket a la cola de espera.
        pthread_mutex_unlock(&mutexJugadores);
        return;
    }

    // Asigna id al nuevo jugador según cantidad conectada.
    int id = jugadoresConectados;
    jugadores[id].socket = socket;        // Guarda el socket.
    jugadores[id].puntos = 0;             // Inicializa puntaje en 0.
    jugadores[id].activo = true;          // Marca como activo.
    jugadores[id].quiereSeguir = true;    // Marca que quiere seguir jugando.
    jugadores[id].intentoHecho = false;   // No ha hecho intento aún.
    jugadores[id].ultimoIntento = -1;     // Último intento inválido al inicio.
    snprintf(jugadores[id].nombre, sizeof(jugadores[id].nombre), "Jugador%d", id+1); // Asigna nombre.

    jugadoresConectados++;                 // Incrementa contador de jugadores conectados.
    pthread_mutex_unlock(&mutexJugadores); // Libera mutex.

    enviarMensaje(socket, "Bienvenido. Esperando inicio de juego...\n"); // Mensaje bienvenida.

    esperarJugadores();                   // Espera condiciones para iniciar el juego.

    // Verifica que el jugador siga activo (no desconectó mientras esperaba).
    pthread_mutex_lock(&mutexJugadores);
    if (!jugadores[id].activo) {
        pthread_mutex_unlock(&mutexJugadores);
        enviarMensaje(socket, "No se pudo unir a la partida.\n");
        close(socket);                    // Cierra socket si no pudo entrar.
        return;
    }
    pthread_mutex_unlock(&mutexJugadores);
    // Bucle principal del juego para este jugador mientras el juego esté activo.
    while (juegoActivo) {
        pthread_mutex_lock(&mutexJuego);

        // Le avisa al jugador que es su turno para enviar un intento.
        enviarMensaje(socket, "Es tu turno para enviar un número (1-10):\n");

        pthread_mutex_unlock(&mutexJuego);

        char buffer[256];
        int leido = read(socket, buffer, sizeof(buffer) - 1); // Lee intento del jugador.
        if (leido <= 0) break;              // Si error o desconexión, sale del loop.

        buffer[leido] = '\0';               // Termina string leído.

        int intento = atoi(buffer);         // Convierte texto a entero.
        if (intento < 1 || intento > 10) { // Valida rango válido.
            enviarMensaje(socket, "Número inválido, debe estar entre 1 y 10.\n");
            continue;                       // Pide otro intento.
        }

        pthread_mutex_lock(&mutexJugadores);
        jugadores[id].ultimoIntento = intento;  // Guarda intento.
        jugadores[id].intentoHecho = true;      // Marca intento hecho.
        pthread_mutex_unlock(&mutexJugadores);

        // Espera que todos hayan hecho intento o timeout 10 segundos.
        pthread_mutex_lock(&mutexJuego);
        int intentosCompletados;
        time_t inicio = time(NULL);

        while (1) {
            intentosCompletados = 1;
            pthread_mutex_lock(&mutexJugadores);
            for (int i = 0; i < MAX_JUGADORES; i++) {
                if (jugadores[i].activo && !jugadores[i].intentoHecho) {
                    intentosCompletados = 0;
                    break;
                }
            }
            pthread_mutex_unlock(&mutexJugadores);

            if (intentosCompletados) break; // Todos intentaron.
            if (time(NULL) - inicio >= 10) break; // Timeout 10s.
            usleep(100000); // Espera 100 ms antes de chequear otra vez.
        }

        // Evaluar intentos para ver si alguien acertó.
        pthread_mutex_lock(&mutexJugadores);
        bool alguienAcerto = false;
        int ganador = -1;
        for (int i = 0; i < MAX_JUGADORES; i++) {
            if (!jugadores[i].activo || !jugadores[i].intentoHecho) continue;
            int val = jugadores[i].ultimoIntento;
            if (val == numeroSecreto) {
                alguienAcerto = true;
                ganador = i;
                jugadores[i].puntos++;          // Suma punto al ganador.
            }
        }
        pthread_mutex_unlock(&mutexJugadores);

        // Envia respuestas individualizadas según si acertó, fue mayor o menor.
        pthread_mutex_lock(&mutexJugadores);
        for (int i = 0; i < MAX_JUGADORES; i++) {
            if (!jugadores[i].activo || !jugadores[i].intentoHecho) continue;
            char resp[50];
            int val = jugadores[i].ultimoIntento;
            if (val == numeroSecreto) {
                snprintf(resp, sizeof(resp), "Acertado! Jugador %d.\n", i+1);
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
            rondaTerminada = true;              // Marca la ronda como terminada.
            pthread_cond_signal(&cond_ronda_terminada); // Señala hilo que espera ronda terminada.
            pthread_mutex_unlock(&mutexJuego);

            break; // Sale del bucle para empezar otra ronda.
        }

        // Resetea intentos para siguiente turno si nadie acertó.
        pthread_mutex_lock(&mutexJugadores);
        for (int i = 0; i < MAX_JUGADORES; i++) {
            if (jugadores[i].activo) jugadores[i].intentoHecho = false;
        }
        pthread_mutex_unlock(&mutexJugadores);

        pthread_mutex_unlock(&mutexJuego);
    }

    // Al salir del juego pregunta si el jugador quiere seguir.
    enviarMensaje(socket, "¿Querés seguir jugando? (s/n):\n");
    char respuesta[10];
    int r = read(socket, respuesta, sizeof(respuesta)-1);
    if (r > 0) {
        respuesta[r] = '\0';
        if (respuesta[0] == 'n' || respuesta[0] == 'N') {
            pthread_mutex_lock(&mutexJugadores);
            jugadores[id].activo = false;       // Marca jugador como inactivo.
            jugadoresConectados--;              // Decrementa contadores.
            pthread_mutex_unlock(&mutexJugadores);
            enviarMensaje(socket, "Gracias por jugar!\n");
            close(socket);                      // Cierra conexión.

            // Si hay jugadores en cola, saca uno para que entre.
            pthread_mutex_lock(&mutexCola);
            if (frenteCola != finalCola) {
                int s = colaEspera[frenteCola];
                frenteCola = (frenteCola + 1) % MAX_COLA_ESPERA;
                pthread_mutex_unlock(&mutexCola);
                manejarConexionJugador(s);    // Maneja nuevo jugador de la cola.
                return;
            }
            pthread_mutex_unlock(&mutexCola);
        }
    }
    close(socket); // Cierra el socket si el jugador no quiere seguir.
}*/

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

// Función que controla el ciclo general del juego: iniciar rondas, esperar fin, etc.
void iniciarJuego() {
    while (juegoActivo) {
        iniciarRonda();  // Genera número secreto y resetea intentos.

        // Espera hasta que la ronda termine (cuando alguien acierta).
        pthread_mutex_lock(&mutexJuego);
        while (!rondaTerminada) {
            pthread_cond_wait(&cond_ronda_terminada, &mutexJuego);
        }
        pthread_mutex_unlock(&mutexJuego);

        sleep(2);  // Espera 2 segundos antes de iniciar la próxima ronda.
    }

    terminarJuego();  // Finaliza el juego, notifica y muestra puntajes.
}
