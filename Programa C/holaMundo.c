#include <stdlib.h>
#include <stdio.h>

int suma ();

int main(int argc, char const *argv[])
{
    printf("La suma es: %d\n", suma());
    system("pause");
    return 0;
}

int suma (){
    int primerNumero, segundoNumero;
    printf("Ingresar primer numero: ");
    scanf("%d", &primerNumero);
    printf("Ingresar segundo numero: ");
    scanf("%d", &segundoNumero);
    return primerNumero+segundoNumero;
}