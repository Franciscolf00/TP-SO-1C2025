#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>


// Constantes: se producirán 10 autos, y el tiempo base por estación es 5 segundos.
#define MAX_AUTOS 10
#define TIEMPO_ESTACION 5
#define ESTACIONES_TRABAJO 5
#define PROBABILIDAD_ACIERTO 90 // Del 1 al 100

// Indices de cada estacion
#define ESTACION_CHASIS 0
#define ESTACION_MOTOR 1
#define ESTACION_VIDRIOS 2
#define ESTACION_INTERIORES 3
#define ESTACION_PINTURA 4

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
#define TIEMPO_CHASIS 8
#define TIEMPO_MOTOR 12
#define TIEMPO_VIDRIOS 3
#define TIEMPO_INTERIORES 10
#define TIEMPO_PINTURA 6

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

    int defectos_por_pieza[ESTACIONES_TRABAJO];       // índice 0: chasis, 1: motor, ...
    float tiempo_por_pieza[ESTACIONES_TRABAJO];       // suma de tiempos por pieza
    float costo_pieza[ESTACIONES_TRABAJO];            // costo acumulado por tipo de pieza
    float costo_defectuosas[ESTACIONES_TRABAJO];      // suma de costos por piezas defectuosas

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
    int costo = PRECIO_CHASIS;
    int tiempo = (rand() % (TIEMPO_CHASIS / 2)) + TIEMPO_CHASIS;
    int cantidad_defectuosos;
    
    if(prob < PROBABILIDAD_ACIERTO)
        printf("Chasis instalado\n");
    while (prob > PROBABILIDAD_ACIERTO)
    {
        printf("chasis defectuoso, cambiando chasis...\n");
        sem_wait(sem); // P()
        stat->partes_defectuosas++;
        stat->tiempo_total_produccion += tiempo;
        stat->costo_total += costo;
        stat->defectos_por_pieza[ESTACION_CHASIS] += 1;
        stat->tiempo_por_pieza[ESTACION_CHASIS] += tiempo;
        stat->costo_pieza[ESTACION_CHASIS] += PRECIO_CHASIS;
        stat->costo_defectuosas[ESTACION_CHASIS] += PRECIO_CHASIS;
        sem_post(sem); // V()
        sleep(TIEMPO_ESTACION); // Opcional

        prob = ((rand() % 100) + 1);

        tiempo = (rand() % (TIEMPO_CHASIS / 2)) + TIEMPO_CHASIS;
    }

    sem_wait(sem); // P()
    stat->costo_total += PRECIO_CHASIS;
    stat->tiempo_total_produccion += tiempo;
    stat->tiempo_por_pieza[ESTACION_CHASIS] += tiempo;
    stat->costo_pieza[ESTACION_CHASIS] += PRECIO_CHASIS;
    sem_post(sem); // V()
}

void motor(t_estadisticas *stat, sem_t *sem)
{
    int prob = (rand() % 100) + 1;

    int tiempo = (rand() % (TIEMPO_MOTOR / 2)) + TIEMPO_MOTOR;


    if(prob<= PROBABILIDAD_ACIERTO)
        printf("motor instalado\n");
    while (prob > PROBABILIDAD_ACIERTO)
    {
        printf("motor defectuoso, cambiando motor...\n");
        sem_wait(sem); // P()
        stat->partes_defectuosas++;
        stat->tiempo_total_produccion += tiempo;
        stat->costo_total += PRECIO_MOTOR;
        stat->defectos_por_pieza[ESTACION_MOTOR] += 1;
        stat->tiempo_por_pieza[ESTACION_MOTOR] += tiempo;
        stat->costo_pieza[ESTACION_MOTOR] += PRECIO_MOTOR;
        stat->costo_defectuosas[ESTACION_MOTOR] += PRECIO_MOTOR;
        sem_post(sem); // V()
        sleep(TIEMPO_ESTACION); 

        prob = (rand() % 100) + 1;

        tiempo = (rand() % (TIEMPO_MOTOR / 2)) + TIEMPO_MOTOR;
    }

    sem_wait(sem); // P()
    stat->costo_total += PRECIO_MOTOR;
    stat->tiempo_total_produccion += tiempo;
    stat->tiempo_por_pieza[ESTACION_MOTOR] += tiempo;
    stat->costo_pieza[ESTACION_MOTOR] += PRECIO_MOTOR;
    sem_post(sem); // V()
}


void vidrios(t_estadisticas *stat, sem_t *sem)
{
    int prob = (rand() % 100) + 1;

    int tiempo = (rand() % (TIEMPO_VIDRIOS / 2)) + TIEMPO_VIDRIOS;

    if(prob<= PROBABILIDAD_ACIERTO)
        printf("vidrios instalado\n");
    while (prob > PROBABILIDAD_ACIERTO)
    {
        printf("vidrios defectuosos, cambiando vidrios...\n");
        sem_wait(sem); // P()
        stat->partes_defectuosas++;
        stat->tiempo_total_produccion += tiempo;
        stat->costo_total += PRECIO_VIDRIOS;
        stat->defectos_por_pieza[ESTACION_VIDRIOS] += 1;
        stat->tiempo_por_pieza[ESTACION_VIDRIOS] += tiempo;
        stat->costo_pieza[ESTACION_VIDRIOS] += PRECIO_VIDRIOS;
        stat->costo_defectuosas[ESTACION_VIDRIOS] += PRECIO_VIDRIOS;
        sem_post(sem); // V()

        sleep(TIEMPO_ESTACION);

        prob = (rand() % 100) + 1;
        tiempo = (rand() % (TIEMPO_VIDRIOS / 2)) + TIEMPO_VIDRIOS;
    }

    sem_wait(sem); // P()
    stat->costo_total += PRECIO_VIDRIOS;
    stat->tiempo_total_produccion += tiempo;
    stat->tiempo_por_pieza[ESTACION_VIDRIOS] += tiempo;
    stat->costo_pieza[ESTACION_VIDRIOS] += PRECIO_VIDRIOS;
    sem_post(sem); // V()
}


void interior(t_estadisticas *stat, sem_t *sem)
{
    int prob = (rand() % 100) + 1;

    int tiempo = (rand() % (TIEMPO_INTERIORES / 2)) + TIEMPO_INTERIORES;

    if(prob<= PROBABILIDAD_ACIERTO)
        printf("Interior instalado\n");
    while (prob > PROBABILIDAD_ACIERTO)
    {
        printf("interiores defectuosos, cambiando interiores...\n");
        sem_wait(sem); // P()
        stat->partes_defectuosas++;
        stat->tiempo_total_produccion += tiempo;
        stat->costo_total += PRECIO_INTERIORES;
        stat->defectos_por_pieza[ESTACION_INTERIORES] += 1;
        stat->tiempo_por_pieza[ESTACION_INTERIORES] += tiempo;
        stat->costo_pieza[ESTACION_INTERIORES] += PRECIO_INTERIORES;
        stat->costo_defectuosas[ESTACION_INTERIORES] += PRECIO_INTERIORES;
        sem_post(sem); // V()

        sleep(TIEMPO_ESTACION);

        prob = (rand() % 100) + 1;

        tiempo = (rand() % (TIEMPO_INTERIORES / 2)) + TIEMPO_INTERIORES;
    }

    sem_wait(sem); // P()
    stat->costo_total += PRECIO_INTERIORES;
    stat->tiempo_total_produccion += tiempo;
    stat->tiempo_por_pieza[ESTACION_INTERIORES] += tiempo;
    stat->costo_pieza[ESTACION_INTERIORES] += PRECIO_INTERIORES;
    sem_post(sem); // V()
}


void pintura(t_estadisticas *stat, sem_t *sem)
{
    int prob = (rand() % 100) + 1;
    
    int tiempo = (rand() % (TIEMPO_PINTURA / 2)) + TIEMPO_PINTURA;

    if(prob<= PROBABILIDAD_ACIERTO)
        printf("Pintura instalado\n");
    while (prob > PROBABILIDAD_ACIERTO)
    {
        printf("pintura defectuosa, cambiando pintura...\n");
        sem_wait(sem); // P()
        stat->partes_defectuosas++;
        stat->tiempo_total_produccion += tiempo;
        stat->costo_total += PRECIO_PINTURA;
        stat->defectos_por_pieza[ESTACION_PINTURA] += 1;
        stat->tiempo_por_pieza[ESTACION_PINTURA] += tiempo;
        stat->costo_pieza[ESTACION_PINTURA] += PRECIO_PINTURA;
        stat->costo_defectuosas[ESTACION_PINTURA] += PRECIO_PINTURA;
        sem_post(sem); // V()

        sleep(TIEMPO_ESTACION);

        prob = (rand() % 100) + 1;
        tiempo = (rand() % (TIEMPO_PINTURA / 2)) + TIEMPO_PINTURA;
    }

    sem_wait(sem); // P()
    stat->costo_total += PRECIO_PINTURA;
    stat->tiempo_total_produccion += tiempo;
    stat->tiempo_por_pieza[ESTACION_PINTURA] += tiempo;
    stat->costo_pieza[ESTACION_PINTURA] += PRECIO_PINTURA;
    sem_post(sem); // V()
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
   

    estadisticas->autos_producidos = 0;
    estadisticas->partes_defectuosas = 0;
    estadisticas->tiempo_total_produccion = 0.0;
    estadisticas->costo_total = 0.0;

for (i = 0; i < ESTACIONES_TRABAJO; i++) {
    estadisticas->defectos_por_pieza[i] = 0;
    estadisticas->tiempo_por_pieza[i] = 0.0;
    estadisticas->costo_pieza[i] = 0.0;
    estadisticas->costo_defectuosas[i] = 0.0;
}


     // Se crea un semáforo con valor inicial 1 (binario) para la memoria comparida.

    sem_t *semaforo = sem_open("semaforo",
                               O_CREAT,
                               0600, // r+w user
                               1);

    // Semaforos para sincronizar los procesos (que no se instale el motor sin chasis y cosas asi...) 
    sem_t *sem_motor = sem_open("sem_motor",
                               O_CREAT,
                               0600, // r+w user
                               0);
            
    sem_t *sem_vidrios = sem_open("sem_vidrios",
                               O_CREAT,
                               0600,
                               0);

    sem_t *sem_interior = sem_open("sem_interior",
                               O_CREAT,
                               0600,
                               0);

    sem_t *sem_pintura = sem_open("sem_pintura",
                               O_CREAT,
                               0600,
                               0);


    // un pid para cada estacion de trabajo, asi tengo un porceso hijo por cada una
    pid_t pids[ESTACIONES_TRABAJO];


    // Cada fase del ensamblaje es realizada por un proceso hijo que itera sobre todos los autos.
    // Se usa fork para cada uno.
    
    // Fase: CHASIS
    if ((pids[ESTACION_CHASIS] = fork()) == 0)
    {
        for (i = 0; i < MAX_AUTOS; i++)
        {
            printf("\n\nAuto: %d\n", i+1);
            chasis(estadisticas, semaforo);
            printf("[PID %d] %s trabajando en auto %d\n", getpid(), "Chasis", i + 1);
            //Compruebo que este en paralelo y no sea todo secuencial
            sem_post(sem_motor);
        }
        exit(0);
    }

    // Fase: MOTOR
    if ((pids[ESTACION_MOTOR] = fork()) == 0)
    {
        for (i = 0; i < MAX_AUTOS; i++)
        {
            sem_wait(sem_motor);
            printf("\n\nAuto: %d\n", i+1);
            motor(estadisticas, semaforo);
            sem_post(sem_vidrios);
        }
        exit(0);
    }

    // Fase: VIDRIOS
    if ((pids[ESTACION_VIDRIOS] = fork()) == 0)
    {
        for (i = 0; i < MAX_AUTOS; i++)
        {
            sem_wait(sem_vidrios);
            printf("\n\nAuto: %d\n", i+1);
            vidrios(estadisticas, semaforo);
            sem_post(sem_interior);
        }
        exit(0);
    }

    // Fase: INTERIOR
    if ((pids[ESTACION_INTERIORES] = fork()) == 0)
    {
        for (i = 0; i < MAX_AUTOS; i++)
        {
            sem_wait(sem_interior);
            printf("\n\nAuto: %d\n", i+1);
            interior(estadisticas, semaforo);
            sem_post(sem_pintura);
        }
        exit(0);
    }

    // Fase: PINTURA
    if ((pids[ESTACION_PINTURA] = fork()) == 0)
    {
        for (i = 0; i < MAX_AUTOS; i++)
        {
            sem_wait(sem_pintura);
            printf("\n\nAuto: %d\n", i+1);
            pintura(estadisticas, semaforo);
        }
        exit(0);
    }
    

    // Espero a todos los hijos para generar el informe
    for (int i = 0; i < ESTACIONES_TRABAJO; i++) {
    waitpid(pids[i], NULL, 0);
}

    // Imprimir resumen
    printf("\nProducción terminada\n");

    printf("Autos producidos: %d\n", MAX_AUTOS);
    printf("\nPartes defectuosas: %d\n", estadisticas->partes_defectuosas);

    printf("Defectos por pieza: \n");
    printf("Chasis defectuosos: %d\n",estadisticas->defectos_por_pieza[ESTACION_CHASIS]);
    printf("Motores defectuosos: %d\n",estadisticas->defectos_por_pieza[ESTACION_MOTOR]);
    printf("Vidrios defectuosos: %d\n",estadisticas->defectos_por_pieza[ESTACION_VIDRIOS]);
    printf("Interiores defectuosos: %d\n",estadisticas->defectos_por_pieza[ESTACION_INTERIORES]);
    printf("Pinturas defectuosas: %d\n",estadisticas->defectos_por_pieza[ESTACION_PINTURA]);
    
    printf("\nTiempo total: %.2f horas\n", estadisticas->tiempo_total_produccion);
    printf("Tiempo por pieza en horas \n");
    printf("Tiempo de chasis: %.2f h\n",estadisticas->tiempo_por_pieza[ESTACION_CHASIS]);
    printf("Tiempo de motores: %.2f h\n",estadisticas->tiempo_por_pieza[ESTACION_MOTOR]);
    printf("Tiempo de vidrios: %.2f h\n",estadisticas->tiempo_por_pieza[ESTACION_VIDRIOS]);
    printf("Tiempo de interiores: %.2f h\n",estadisticas->tiempo_por_pieza[ESTACION_INTERIORES]);
    printf("Tiempo de pintura: %.2f h\n",estadisticas->tiempo_por_pieza[ESTACION_PINTURA]);

    printf("\nCosto total: %.2f $\n", estadisticas->costo_total);
    printf("Costo por pieza:\n");
    printf("Costo de chasis: %.2f $\n",estadisticas->costo_pieza[ESTACION_CHASIS]);
    printf("Costo de motores: %.2f $\n",estadisticas->costo_pieza[ESTACION_MOTOR]);
    printf("Costo de vidrios: %.2f $\n",estadisticas->costo_pieza[ESTACION_VIDRIOS]);
    printf("Costo de interiores: %.2f $\n",estadisticas->costo_pieza[ESTACION_INTERIORES]);
    printf("Costo de pintura: %.2f $\n",estadisticas->costo_pieza[ESTACION_PINTURA]);
    printf("Costo por pieza defectuosa:\n");
    printf("Costo de chasis defectuosos: %.2f $\n",estadisticas->costo_defectuosas[ESTACION_CHASIS]);
    printf("Costo de motores defectuosos: %.2f $\n",estadisticas->costo_defectuosas[ESTACION_MOTOR]);
    printf("Costo de vidrios defectuosos: %.2f $\n",estadisticas->costo_defectuosas[ESTACION_VIDRIOS]);
    printf("Costo de interiores defectuosos: %.2f $\n",estadisticas->costo_defectuosas[ESTACION_INTERIORES]);
    printf("Costo de pintura defectuosas: %.2f $\n",estadisticas->costo_defectuosas[ESTACION_PINTURA]);


    // Liberar recursos, se desconecta y elimina la memoria compartida y el semáforo.

    munmap(estadisticas, sizeof(t_estadisticas));

    sem_close(semaforo);
    sem_close(sem_motor);
    sem_close(sem_vidrios);
    sem_close(sem_interior);
    sem_close(sem_pintura);


    sem_unlink("semaforo");
    sem_unlink("sem_motor");
    sem_unlink("sem_vidrios");
    sem_unlink("sem_interior");
    sem_unlink("sem_pintura");

    munmap(estadisticas, sizeof(t_estadisticas));
    shm_unlink(NombreMemoria);

    return EXIT_SUCCESS;
}
