#include <xc.h>

#pragma config OSC= HS, WDT = OFF, BOREN = OFF

#define _XTAL_FREQ 16000000  // Frequência do clock em Hz

void main() {
    TRISD = 0x00;  // Configura RD0 a RD7 como saídas
    PORTD = 0x00;  // Inicializa RD0 a RD7 como 0

    unsigned char binaryValue = 0;  // Inicializa o valor binário como 0
    unsigned char pressedRB0 = 0;  // Flag para indicar se RB0 foi pressionado
    unsigned char pressedRB1 = 0;  // Flag para indicar se RB1 foi pressionado

    while(1) {
        // Incrementa o valor binário quando RB0 é pressionado
        if (PORTBbits.RB0 == 0 && pressedRB0 == 0) {
            __delay_ms(20); // Aguarda um curto período para evitar múltiplas leituras do botão
            if (PORTBbits.RB0 == 0) { // Verifica se o botão ainda está pressionado
                if (binaryValue >= 255){
                    binaryValue = 255;  // Limita o valor binário em 0 a 255
                } else{
                    binaryValue++;  // Incrementa o valor binário
                }
                pressedRB0 = 1; // Marca que o botão já foi pressionado
            }
        } else if (PORTBbits.RB0 == 1) {
            pressedRB0 = 0;  // Limpa a flag de pressionamento de RB0
        }

        // Decrementa o valor binário quando RB1 é pressionado
        if (PORTBbits.RB1 == 0 && pressedRB1 == 0) {
            __delay_ms(20); // Aguarda um curto período para evitar múltiplas leituras do botão
            if (PORTBbits.RB1 == 0) { // Verifica se o botão ainda está pressionado
                if (binaryValue <= 0){
                    binaryValue = 0;  // Limita o valor binário em 0 a 255
                }else{
                    binaryValue--;  // Decrementa o valor binário
                }
                pressedRB1 = 1; // Marca que o botão já foi pressionado
            }
        } else if (PORTBbits.RB1 == 1) {
            pressedRB1 = 0;  // Limpa a flag de pressionamento de RB1
        }

        // Exibe o valor binário nos LEDs
        PORTD = binaryValue;  // Exibe o valor binário diretamente nos pinos RD0 a RD7
        __delay_ms(5);  // Aguarda um curto período para garantir que os LEDs sejam atualizados
    }
}
