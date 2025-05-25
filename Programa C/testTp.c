#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>


// CAMBIAR LOS PRINT POR COUT?? PREGUNTAR SI ES NECESARIO

// Constantes: se producirán 10 autos, y el tiempo base por estación es 5 segundos.
#define MAX_AUTOS 10
#define TIEMPO_ESTACION 5

// Constantes de Precio y Tiempo cualquier cosa se cambia desde aca
// revisar el tema de la randomizacion en precios y tiempo
#define PRECIO_CHASIS 5000
#define PRECIO_MOTOR 7000
#define PRECIO_VIDRIOS 3000
#define PRECIO_INTERIORES 2000
#define PRECIO_PINTURA 1500

// Tiempo, por ahora lo dejo en segundos, eso lo cambiamos despues desde aca tambien
// Revisar si el tiempo es variable o no
// de momento dejo todo en 5
#define TIEMPO_CHASIS 5
#define TIEMPO_MOTOR 5
#define TIEMPO_VIDRIOS 5
#define TIEMPO_INTERIORES 5
#define TIEMPO_PINTURA 5

// Memoria compartda
#define NombreMemoria "miMemoria"

// IMPORTANTE
// INTENTAR GENERALIZAR EL TEMA DE LA PROBABILIDAD DE FALLO

// stats globales de la produccion
typedef struct
{
    int autos_producidos;
    int partes_defectuosas;
    float tiempo_total_produccion;
    float costo_total;
} t_estadisticas;

/////////////////////////////////////////
/////////////////////////////////////////


/*
Funciones de producción por etapa
Cada función sigue un patrón parecido:

Genera un valor aleatorio de fallo.

Si hay defecto, reintenta con penalización de tiempo y costo.

Acumula los valores en las estadísticas globales con protección del semáforo.
*/

void chasis(t_estadisticas *stat, sem_t *sem)
{
    int prob = (rand() % 100) + 1;
    // Precio y tiempo variable, Si no lo vamos a usarm simplemente hacemos += Constante de precio/tiempo
    // Se repite dentro del while si se cambia, hay que hacerlo en ambos lados
    // Se borran las variables costo y tiempo y se suman las constantes directamente en la variable estadisticas
    // En todo caso hay que cambiarlo en todas las estaciones de trabajo
    int costo = (rand() % (PRECIO_CHASIS / 2)) + PRECIO_CHASIS;
    int tiempo = (rand() % (TIEMPO_CHASIS / 2)) + TIEMPO_CHASIS;

    while (prob > 90)
    {
        printf("chasis defectuoso, cambiando chasis...\n");
        // sem V
        stat->partes_defectuosas++;
        stat->tiempo_total_produccion += tiempo;
        stat->costo_total += costo;
        // sem P
        sleep(TIEMPO_ESTACION); // Opcional

        prob = ((rand() % 100) + 1);

        costo = (rand() % (PRECIO_CHASIS / 2)) + PRECIO_CHASIS;
        tiempo = (rand() % (TIEMPO_CHASIS / 2)) + TIEMPO_CHASIS;
    }

    // sem V
    stat->costo_total += costo;
    stat->tiempo_total_produccion += tiempo;
    // Sem P
}

void motor(t_estadisticas *stat, sem_t *sem)
{
    int prob = (rand() % 100) + 1;

    int costo = (rand() % (PRECIO_MOTOR / 2)) + PRECIO_MOTOR;
    int tiempo = (rand() % (TIEMPO_MOTOR / 2)) + TIEMPO_MOTOR;

    while (prob > 90)
    {
        printf("motor defectuoso, cambiando motor...\n");
        // sem V
        stat->partes_defectuosas++;
        stat->tiempo_total_produccion += tiempo;
        stat->costo_total += costo;
        // Sem P
        sleep(TIEMPO_ESTACION);

        prob = (rand() % 100) + 1;

        costo = (rand() % (PRECIO_MOTOR / 2)) + PRECIO_MOTOR;
        tiempo = (rand() % (TIEMPO_MOTOR / 2)) + TIEMPO_MOTOR;
    }

    // sem V
    stat->costo_total += costo;
    stat->tiempo_total_produccion += tiempo;
    // Sem P
}


void vidrios(t_estadisticas *stat, sem_t *sem)
{
    int prob = (rand() % 100) + 1;

    int costo = (rand() % (PRECIO_VIDRIOS / 2)) + PRECIO_VIDRIOS;
    int tiempo = (rand() % (TIEMPO_VIDRIOS / 2)) + TIEMPO_VIDRIOS;

    while (prob > 90)
    {
        printf("vidrios defectuosos, cambiando vidrios...\n");
        // sem V
        stat->partes_defectuosas++;
        stat->tiempo_total_produccion += tiempo;
        stat->costo_total += costo;
        // Sem P

        sleep(TIEMPO_ESTACION);

        prob = (rand() % 100) + 1;
        costo = (rand() % (PRECIO_VIDRIOS / 2)) + PRECIO_VIDRIOS;
        tiempo = (rand() % (TIEMPO_VIDRIOS / 2)) + TIEMPO_VIDRIOS;
    }

    // sem V
    stat->costo_total += costo;
    stat->tiempo_total_produccion += tiempo;
    // Sem P
}


void interior(t_estadisticas *stat, sem_t *sem)
{
    int prob = (rand() % 100) + 1;

    int costo = (rand() % (PRECIO_INTERIORES / 2)) + PRECIO_INTERIORES;
    int tiempo = (rand() % (TIEMPO_INTERIORES / 2)) + TIEMPO_INTERIORES;

    while (prob > 90)
    {
        printf("interiores defectuosos, cambiando interiores...\n");
        // sem V
        stat->partes_defectuosas++;
        stat->tiempo_total_produccion += tiempo;
        stat->costo_total += costo;
        // Sem P

        sleep(TIEMPO_ESTACION);

        prob = (rand() % 100) + 1;

        costo = (rand() % (PRECIO_INTERIORES / 2)) + PRECIO_INTERIORES;
        tiempo = (rand() % (TIEMPO_INTERIORES / 2)) + TIEMPO_INTERIORES;
    }

    // sem V
    stat->costo_total += costo;
    stat->tiempo_total_produccion += tiempo;
    // Sem P
}


void pintura(t_estadisticas *stat, sem_t *sem)
{
    int prob = (rand() % 100) + 1;
    
    int costo = (rand() % (PRECIO_PINTURA / 2)) + PRECIO_PINTURA;
    int tiempo = (rand() % (TIEMPO_PINTURA / 2)) + TIEMPO_PINTURA;

    while (prob > 90)
    {
        printf("pintura defectuosa, cambiando pintura...\n");
        // sem V
        stat->partes_defectuosas++;
        stat->tiempo_total_produccion += tiempo;
        stat->costo_total += costo;
        // Sem P

        sleep(TIEMPO_ESTACION);

        prob = (rand() % 100) + 1;
        costo = (rand() % (PRECIO_PINTURA / 2)) + PRECIO_PINTURA;
        tiempo = (rand() % (TIEMPO_PINTURA / 2)) + TIEMPO_PINTURA;
    }

    // sem V
    stat->costo_total += costo;
    stat->tiempo_total_produccion += tiempo;
    // Sem P
}



int main()
{
    srand(time(NULL));
    int i;

    // Se crean e inicializan las partes de memoria compartida para las estadísticas.

    // Memoria compartida para estadísticas (segun los pasos que vimos en clase)
    int idMemoria = shm_open(NombreMemoria,
                            O_CREAT | O_RDWR,
                            0600);

    ftruncate(idMemoria, sizeof(t_estadisticas));
    t_estadisticas *estadisticas = (t_estadisticas*) mmap(NULL,
                                                            sizeof(t_estadisticas),
                                                            PROT_READ | PROT_WRITE,
                                                            MAP_SHARED,
                                                            idMemoria,
                                                            0);
    
    close(idMemoria);

    // Se crea un semáforo con valor inicial 1 (binario).
    
    sem_t *semaforo = sem_open("semaforo",
                               O_CREAT,
                               0600, // r+w user
                               1);

    pid_t pid;

    /*
    Cada fase del ensamblaje es realizada por un proceso hijo que itera sobre todos los autos.
    Se usa fork para cada uno, y waitpid para esperar su finalización antes de pasar a la siguiente etapa.
    */

    // Fase: CHASIS
    if ((pid = fork()) == 0)
    {
        for (i = 0; i < MAX_AUTOS; i++)
        {
            chasis(estadisticas, semaforo);
        }
        exit(0);
    }
    waitpid(pid, NULL, 0);

    // Fase: MOTOR
    if ((pid = fork()) == 0)
    {
        for (i = 0; i < MAX_AUTOS; i++)
        {
            motor(estadisticas, semaforo);
        }
        exit(0);
    }
    waitpid(pid, NULL, 0);

    // Fase: VIDRIOS
    if ((pid = fork()) == 0)
    {
        for (i = 0; i < MAX_AUTOS; i++)
        {
            vidrios(estadisticas, semaforo);
        }
        exit(0);
    }
    waitpid(pid, NULL, 0);

    // Fase: INTERIOR
    if ((pid = fork()) == 0)
    {
        for (i = 0; i < MAX_AUTOS; i++)
        {
            interior(estadisticas, semaforo);
        }
        exit(0);
    }
    waitpid(pid, NULL, 0);

    // Fase: PINTURA
    if ((pid = fork()) == 0)
    {
        for (i = 0; i < MAX_AUTOS; i++)
        {
            pintura(estadisticas, semaforo);
        }
        exit(0);
    }
    waitpid(pid, NULL, 0);

    // Imprimir resumen
    printf("\nProducción terminada\n");
    printf("Autos producidos: %d\n", MAX_AUTOS);
    printf("Partes defectuosas: %d\n", estadisticas->partes_defectuosas);
    printf("Costo total: %.2f $\n", estadisticas->costo_total);
    printf("Tiempo total: %.2f\n\n", estadisticas->tiempo_total_produccion);


    // Liberar recursos, se desconecta y elimina la memoria compartida y el semáforo.

    sem_close(semaforo);
    munmap(estadisticas, sizeof(t_estadisticas));

    return EXIT_SUCCESS;
}
