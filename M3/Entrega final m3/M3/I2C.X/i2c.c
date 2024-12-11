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
float frequenciaAtual = 102.1;
float frequenciaSalva = 102.1; // Variável para salvar a frequência antes de desligar
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
void salvarFavorito(unsigned char);
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
    while (1) {
        verificarBotoes();
    }
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
        TEA5767_Sintonizar(frequenciaAtual);
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

void ajustarFrequencia(unsigned char incremento) {
    float novaFrequencia = frequenciaAtual;
    unsigned char sinalValido = 0;

    while (!sinalValido) {
        if (incremento) {
            novaFrequencia += 0.1;
            if (novaFrequencia > 108.0) {
                novaFrequencia = 87.5;
            }
        } else {
            novaFrequencia -= 0.1;
            if (novaFrequencia < 87.5) {
                novaFrequencia = 108.0;
            }
        }
        TEA5767_Sintonizar(novaFrequencia);
        __delay_ms(500); // Aguarda um pouco para sintonizar

        // Verifica a qualidade do sinal
        sinalValido = verificarQualidadeSinal();
        printf("Qualidade do Sinal: %d\n", qualidadeSinal);
    }

    frequenciaAtual = novaFrequencia;
    atualizarLCD();
}

void ajustarVolume(unsigned char incremento) {
    // Ajustar o volume (TEA5767 não tem controle de volume por I2C)
}

void salvarFavorito(unsigned char favorito) {
    // Salvar a frequência atual como favorita
    if (favorito == 1) {
        favorito1 = frequenciaAtual;
    } else if (favorito == 2) {
        favorito2 = frequenciaAtual;
    }
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

unsigned char verificarQualidadeSinal(void) {
    unsigned char status[5];

    // Leitura dos dados do TEA5767 para verificar a qualidade do sinal
    I2C_START();
    I2C_TRANSMITE((TEA5767_ADDRESS << 1) | 1);
    for (int i = 0; i < 5; i++) {
        status[i] = I2C_RECEBE();
        if (i < 4) {
            I2C_ACK();
        } else {
            I2C_NACK();
        }
    }
    I2C_STOP();

    qualidadeSinal = status[3]; // Qualidade do sinal está no terceiro byte

    // Ajusta o LED RGB com base na qualidade do sinal
    if (qualidadeSinal < 131) {  // Sinal ruim
        LED_RGB_R = 1;
        LED_RGB_G = 0;
        LED_RGB_B = 0;
        return 0;
    } else if (qualidadeSinal < 180) {  // Sinal fraco
        LED_RGB_R = 1;
        LED_RGB_G = 1;
        LED_RGB_B = 0;
        return 1;
    } else if (qualidadeSinal < 210) {  // Sinal médio
        LED_RGB_R = 1;
        LED_RGB_G = 1;
        LED_RGB_B = 0;
        return 1;
    } else if (qualidadeSinal > 900) {  // Sinal médio
        LED_RGB_R = 1;
        LED_RGB_G = 0;
        LED_RGB_B = 0;
        return 0;
    } else {  // Sinal bom
        LED_RGB_R = 0;
        LED_RGB_G = 1;
        LED_RGB_B = 0;
        return 1;
    }
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

void verificarFavorito(unsigned char botao, float* favorito, unsigned char* pressed) {
    if (botao == 0) {
        if (!*pressed) {
            pressStartTime = millis();
            *pressed = 1;
        } else if (millis() - pressStartTime >= 3000) {
            // Se o botão foi pressionado por 3 segundos, salva a frequência atual
            salvarFavorito(botao == BOTAO_FAVORITO1 ? 1 : 2);
            *pressed = 0;
        }
    } else {
        if (*pressed) {
            if (millis() - pressStartTime < 3000) {
                // Se o botão foi pressionado por menos de 3 segundos, sintoniza na frequência favorita
                if (*favorito > 0) {
                    frequenciaAtual = *favorito;
                    TEA5767_Sintonizar(frequenciaAtual);
                    atualizarLCD();
                }
            }
            *pressed = 0;
        }
    }
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

void verificarBotoes(void) {
    // Verifica o botão de liga/desliga
    if (BOTAO_LIGA_DESLIGA == 0 && !pressedRB0) {
        __delay_ms(20);
        if (BOTAO_LIGA_DESLIGA == 0) { // Verifica se o botão ainda está pressionado
            if (radioLigado) {
                desligarRadio();
            } else {
                ligarRadio();
            }
            pressedRB0 = 1;
        }
    } else if (BOTAO_LIGA_DESLIGA == 0) {
        __delay_ms(20);
        if (radioLigado){
            desligarRadio();
        }
    } else if (BOTAO_LIGA_DESLIGA == 1) {
        pressedRB0 = 0;
    }

    // Se o rádio estiver ligado, verifica os outros botões
    if (radioLigado) {
        if (BOTAO_LIGA_DESLIGA == 0 && pressedRB0 == 0) {
            __delay_ms(20);
            if (radioLigado){
                desligarRadio();
            }
        }
        if (BOTAO_AUMENTA_FREQUENCIA == 0 && !pressedRB1) {
            __delay_ms(20);
            if (BOTAO_AUMENTA_FREQUENCIA == 0) { // Verifica se o botão ainda está pressionado
                ajustarFrequencia(1);
                pressedRB1 = 1;
            }
        } else if (BOTAO_AUMENTA_FREQUENCIA == 1) {
            pressedRB1 = 0;
        }

        if (BOTAO_DIMINUI_FREQUENCIA == 0 && !pressedRB2) {
            __delay_ms(20);
            if (BOTAO_DIMINUI_FREQUENCIA == 0) { // Verifica se o botão ainda está pressionado
                ajustarFrequencia(0);
                pressedRB2 = 1;
            }
        } else if (BOTAO_DIMINUI_FREQUENCIA == 1) {
            pressedRB2 = 0;
        }

        if (BOTAO_AUMENTA_VOLUME == 0 && !pressedRB3) {
            __delay_ms(20);
            if (BOTAO_AUMENTA_VOLUME == 0) { // Verifica se o botão ainda está pressionado
                ajustarVolume(1);
                pressedRB3 = 1;
            }
        } else if (BOTAO_AUMENTA_VOLUME == 1) {
            pressedRB3 = 0;
        }

        if (BOTAO_DIMINUI_VOLUME == 0 && !pressedRB4) {
            __delay_ms(20);
            if (BOTAO_DIMINUI_VOLUME == 0) { // Verifica se o botão ainda está pressionado
                ajustarVolume(0);
                pressedRB4 = 1;
            }
        } else if (BOTAO_DIMINUI_VOLUME == 1) {
            pressedRB4 = 0;
        }

        verificarFavorito(BOTAO_FAVORITO1, &favorito1, &pressedRB5);
        verificarFavorito(BOTAO_FAVORITO2, &favorito2, &pressedRB6);
    }
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