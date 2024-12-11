#include <xc.h>
#include <stdio.h>
#include <math.h>  // Para a função pow
#include "I2C_Master.h"
#include "TEA5767.h"
#include "LCD.h"
#include "utils.h" 

// Configuração do microcontrolador
#pragma config OSC = HS
#pragma config WDT = OFF
#pragma config BOREN = OFF

#define _XTAL_FREQ 16000000
#define TEA5767_ADDRESS 0x60

// Definições de pinos
#define LED_VERDE LATDbits.LATD0
#define LED_VERMELHO LATDbits.LATD1
#define LED_RGB_R LATDbits.LATD2
#define LED_RGB_G LATDbits.LATD3
#define LED_RGB_B LATDbits.LATD4

#define BOTAO_LIGA_DESLIGA PORTBbits.RB0
#define BOTAO_AUMENTA_FREQUENCIA PORTBbits.RB1
#define BOTAO_DIMINUI_FREQUENCIA PORTBbits.RB2
#define BOTAO_AUMENTA_VOLUME PORTBbits.RB3
#define BOTAO_DIMINUI_VOLUME PORTBbits.RB4
#define BOTAO_FAVORITO1 PORTBbits.RB5
#define BOTAO_FAVORITO2 PORTBbits.RB6

#define BUZZER LATDbits.LATD7  // Define o pino RD7 como o buzzer

// Definições do LCD
#define LCD_ADDR 0x27 // Endereço I2C do display LCD
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

#define En 0b00000100 // Enable bit
#define Rw 0b00000010 // Read/Write bit
#define Rs 0b00000001 // Register select bit

// Variáveis globais
unsigned char radioLigado = 0;
unsigned char pressedRB0 = 0;
unsigned char pressedRB1 = 0;
unsigned char pressedRB2 = 0;
unsigned char pressedRB3 = 0;
unsigned char pressedRB4 = 0;
unsigned char pressedRB5 = 0;
unsigned char pressedRB6 = 0;
unsigned char qualidadeSinal = 0;
float frequenciaAtual = 94.1;
float frequenciaSalva = 94.1; // Variável para salvar a frequência antes de desligar
float favorito1 = 0;
float favorito2 = 0;
unsigned long pressStartTime = 0;
unsigned char qualidadeSinal;
float temperaturaAtual;

// Protótipos das funções
void inicializarSistema(void);
void verificarBotoes(void);
void ligarRadio(void);
void desligarRadio(void);
void ajustarFrequencia(unsigned char);
void ajustarVolume(unsigned char);
void sintonizarFrequenciaEspecifica(float frequencia);
void atualizarLCD(void);
unsigned char verificarQualidadeSinal(void);
void desligarTEA5767(void);
void verificarFavorito(unsigned char, float*, unsigned char*);
unsigned long millis(void);
void inicializarSensorLM75(void);
float lerTemperatura(void);

// Funções para o display LCD
void lcd_send_nibble(char nibble);
void lcd_send_byte(char rs, char data);
void lcd_init(void);
void lcd_set_cursor(char row, char col);
void lcd_print(const char *str);
void lcd_clear(void);

void Buzzer_On() {
    BUZZER = 1;  // Ativa o buzzer
    __delay_ms(100);  // Ativa o buzzer por 100 ms
    BUZZER = 0;  // Desativa o buzzer
}

void main() {
    inicializarSistema();
    ligarRadio();
    while (1){
        
    }
    __delay_ms(500);
    
}

void inicializarSistema(void) {
    TRISD = 0x00;  // Configura RD0 a RD7 como saídas
    PORTD = 0x00;  // Inicializa RD0 a RD7 como 0

    TRISB = 0xFF;  // Configura RB0 a RB7 como entradas
    PORTB = 0x00;  // Inicializa RB0 a RB7 como 0

    // Inicializa o LCD e outros periféricos
    I2C_INICIA();
    inicializarSensorLM75();
    
    // Inicializa o estado dos LEDs
    LED_VERDE = 0;
    LED_VERMELHO = 1;  // LED vermelho aceso ao iniciar desligado
    LED_RGB_R = 0;
    LED_RGB_G = 0;
    LED_RGB_B = 0;
    
    BUZZER = 0;  // Garante que o buzzer está desligado inicialmente
    
    // Chama a função para ativar o buzzer na inicialização
    Buzzer_On();
}

void ligarRadio(void) {
    if (!radioLigado) {  // Apenas liga se o rádio não estiver ligado
        radioLigado = 1;
        LED_VERDE = 1;
        LED_VERMELHO = 0;
        lcd_init();
        lcd_clear();
        frequenciaAtual = frequenciaSalva; // Restaura a frequência salva
        atualizarLCD();
        Buzzer_On();
    }
}

void desligarRadio(void) {
    radioLigado = 0;
    LED_VERDE = 0;
    LED_VERMELHO = 1;
    frequenciaSalva = frequenciaAtual; // Salva a frequência atual
    lcd_clear();
    desligarTEA5767();
    Buzzer_On();
}

void atualizarLCD(void) {
    temperaturaAtual = lerTemperatura();
    lcd_set_cursor(0, 0);
    lcd_print("Freq:");
    char buffer[16];
    sprintf(buffer, "%3.1f MHz", frequenciaAtual);
    lcd_print(buffer);
    lcd_set_cursor(1, 0);
    lcd_print("T:");
    sprintf(buffer, "%2.2fC", temperaturaAtual);
    lcd_print(buffer);
    lcd_print(" - Q:");
    sprintf(buffer, "%d", qualidadeSinal);
    lcd_print(buffer);
}


void desligarTEA5767(void) {
    // Envia um comando para colocar o TEA5767 em modo idle ou desligado
    I2C_START();
    I2C_TRANSMITE(TEA5767_ADDRESS << 1);
    I2C_TRANSMITE(0x00); // Comando de desligamento ou idle
    I2C_TRANSMITE(0x00);
    I2C_TRANSMITE(0x00);
    I2C_TRANSMITE(0x00);
    I2C_TRANSMITE(0x00);
    I2C_STOP();
}

unsigned long millis(void) {
    return (unsigned long)((TMR0H << 8) | TMR0L);
}

void inicializarSensorLM75(void) {
    // Inicializa o sensor LM75
    I2C_START();
    I2C_TRANSMITE(0x90); // Endereço do LM75 com bit de escrita
    I2C_TRANSMITE(0x01); // Ponteiro de configuração
    I2C_TRANSMITE(0x00); // Configuração: modo normal
    I2C_STOP();
}

float lerTemperatura(void) {
    unsigned char temp_high, temp_low;
    int temp;

    I2C_START();
    I2C_TRANSMITE(0x90); // Endereço do LM75 com bit de escrita
    I2C_TRANSMITE(0x00); // Ponteiro de temperatura
    I2C_RESTART();
    I2C_TRANSMITE(0x91); // Endereço do LM75 com bit de leitura
    temp_high = I2C_RECEBE();
    I2C_ACK();
    temp_low = I2C_RECEBE();
    I2C_NACK();
    I2C_STOP();

    temp = (temp_high << 8) | temp_low;
    return (float)(temp / 256.0);
}

// Funções para o display LCD
void lcd_send_nibble(char nibble) {
    I2C_START();
    I2C_TRANSMITE(LCD_ADDR << 1);
    I2C_TRANSMITE(nibble | LCD_BACKLIGHT);
    I2C_STOP();
    __delay_us(50);
    I2C_START();
    I2C_TRANSMITE(LCD_ADDR << 1);
    I2C_TRANSMITE((nibble | En | LCD_BACKLIGHT));
    I2C_STOP();
    __delay_us(50);
    I2C_START();
    I2C_TRANSMITE(LCD_ADDR << 1);
    I2C_TRANSMITE((nibble & ~En) | LCD_BACKLIGHT);
    I2C_STOP();
    __delay_us(50);
}

void lcd_send_byte(char rs, char data) {
    lcd_send_nibble(rs | (data & 0xF0));
    lcd_send_nibble(rs | ((data << 4) & 0xF0));
}

void lcd_init(void) {
    __delay_ms(50);
    lcd_send_nibble(0x30);
    __delay_ms(5);
    lcd_send_nibble(0x30);
    __delay_us(200);
    lcd_send_nibble(0x30);
    __delay_us(50);
    lcd_send_nibble(0x20);
    __delay_us(50);
    lcd_send_byte(0, 0x28);
    __delay_us(50);
    lcd_send_byte(0, 0x08);
    __delay_us(50);
    lcd_send_byte(0, 0x01);
    __delay_ms(2);
    lcd_send_byte(0, 0x06);
    __delay_us(50);
    lcd_send_byte(0, 0x0C);
}

void lcd_set_cursor(char row, char col) {
    char address;
    if (row == 0) {
        address = 0x80 + col;
    } else {
        address = 0xC0 + col;
    }
    lcd_send_byte(0, address);
}

void lcd_print(const char *str) {
    while (*str) {
        lcd_send_byte(Rs, *str++);
    }
}

void lcd_clear(void) {
    lcd_send_byte(0, 0x01);
    __delay_ms(2);
}
