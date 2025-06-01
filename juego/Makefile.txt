CC = gcc
CFLAGS = -Wall -pthread

all: servidor cliente

servidor: main.o funciones.o variablesglobales.o
	$(CC) $(CFLAGS) -o servidor main.o funciones.o variablesglobales.o

cliente: cliente.o
	$(CC) $(CFLAGS) -o cliente cliente.o

main.o: main.c funciones.h variablesglobal.h
	$(CC) $(CFLAGS) -c main.c

funciones.o: funciones.c funciones.h variablesglobal.h
	$(CC) $(CFLAGS) -c funciones.c

variablesglobales.o: variablesglobales.c variablesglobal.h
	$(CC) $(CFLAGS) -c variablesglobales.c

cliente.o: cliente.c
	$(CC) $(CFLAGS) -c cliente.c

clean:
	rm -f *.o servidor cliente
