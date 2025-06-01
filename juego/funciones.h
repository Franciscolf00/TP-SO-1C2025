#ifndef FUNCIONES_H
#define FUNCIONES_H

//void manejarConexionJugador(int socketCliente);
void iniciarJuego();
void enviarMensaje(int socket, const char* mensaje);
void notificarTodos(const char* mensaje);
void mostrarTablaPuntajes();
void agregarACola(int socketCliente);
void verificarInicioJuego();
void esperarJugadores();
void iniciarRonda();
void terminarJuego();

#endif
