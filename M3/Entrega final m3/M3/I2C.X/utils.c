#include <stdio.h>  // Para sprintf
#include <math.h>   // Para pow
#include "utils.h"

void my_ftoa(char *buffer, float valor, unsigned char casasDecimais) {
    int inteiro = (int)valor;
    int fracionario = (int)((valor - inteiro) * pow(10, casasDecimais));
    sprintf(buffer, "%d.%0*d", inteiro, casasDecimais, fracionario);
}

void my_itoa(char *buffer, int valor) {
    sprintf(buffer, "%d", valor);
}
