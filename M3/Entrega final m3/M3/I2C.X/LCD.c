#include <xc.h>
#include <stdio.h>
#include "I2C_Master.h"

#define LCD_ADDRESS 0x27
#define LCD_BACKLIGHT 0x08 
#define LCD_NOBACKLIGHT 0x00

#define _XTAL_FREQ 16000000 // Certifique-se de que essa constante esteja definida

void I2C_EnviarNibble(unsigned char nibble); // Declaração da função

void LCD_Inicializar(void) {
    __delay_ms(50); // Espera o LCD inicializar

    I2C_START();
    I2C_TRANSMITE(LCD_ADDRESS << 1);

    I2C_EnviarNibble(0x30);
    __delay_ms(5);
    I2C_EnviarNibble(0x30);
    __delay_us(150);
    I2C_EnviarNibble(0x30);
    I2C_EnviarNibble(0x20); // 4-bit mode

    I2C_EnviarNibble(0x20); // Function set
    I2C_EnviarNibble(0x80);

    I2C_EnviarNibble(0x00); // Display control
    I2C_EnviarNibble(0xC0);

    I2C_EnviarNibble(0x00); // Clear display
    I2C_EnviarNibble(0x10);
    __delay_ms(2);

    I2C_EnviarNibble(0x00); // Entry mode set
    I2C_EnviarNibble(0x60);

    I2C_STOP();
}

void LCD_Limpar(void) {
    I2C_START();
    I2C_TRANSMITE(LCD_ADDRESS << 1);
    I2C_TRANSMITE(0x00); // Comando de controle
    I2C_TRANSMITE(0x01); // Comando para limpar a tela
    I2C_STOP();
    __delay_ms(2); // Delay necessário para o comando ser executado
}

void LCD_DefinirCursor(unsigned char linha, unsigned char coluna) {
    unsigned char comando;
    if (linha == 0) {
        comando = 0x80 + coluna;
    } else if (linha == 1) {
        comando = 0xC0 + coluna;
    }

    I2C_START();
    I2C_TRANSMITE(LCD_ADDRESS << 1);
    I2C_TRANSMITE(0x00); // Comando de controle
    I2C_TRANSMITE(comando); // Comando para definir o cursor na posição especificada
    I2C_STOP();
}

void LCD_EscreverString(char* str) {
    I2C_START();
    I2C_TRANSMITE(LCD_ADDRESS << 1);
    while (*str) {
        I2C_TRANSMITE(0x40); // Comando de controle para dados
        I2C_TRANSMITE(*str++);
    }
    I2C_STOP();
}

void I2C_EnviarNibble(unsigned char nibble) {
    I2C_TRANSMITE(nibble | 0x08); // Envia o nibble com o bit de habilitação de dados
    I2C_TRANSMITE(nibble & ~0x08); // Envia o nibble sem o bit de habilitação de dados
}

void LCD_EscreverNumero(int numero) {
    char buffer[16];
    sprintf(buffer, "%d", numero);
    LCD_EscreverString(buffer);
}

void LCD_EscreverFloat(float numero, unsigned char casasDecimais) {
    char buffer[16];
    sprintf(buffer, "%.*f", casasDecimais, numero);
    LCD_EscreverString(buffer);
}