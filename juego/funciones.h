#ifndef FUNCIONES_H
#define FUNCIONES_H

void manejarConexionJugador(int socketCliente);
void iniciarJuego();
void enviarMensaje(int socket, const char* mensaje);
void notificarTodos(const char* mensaje);
void mostrarTablaPuntajes();
void agregarACola(int socketCliente);
void verificarInicioJuego();
void esperarJugadores();
void iniciarRonda();
void terminarJuego();
// Verifica si todos los jugadores hicieron su intento o si se agotó el tiempo de espera
bool todosIntentaronOTimeout();

// Espera hasta que todos los jugadores hagan sus intentos o se agote el timeout
void esperarTodosIntentos();

// Evalúa los intentos de todos los jugadores y determina ganadores
void evaluarIntentos();

// Verifica si todos los jugadores hicieron su intento o si se agotó el tiempo de espera
bool todosIntentaronOTimeout();

// Espera hasta que todos los jugadores hagan sus intentos o se agote el timeout
void esperarTodosIntentos();


#endif
