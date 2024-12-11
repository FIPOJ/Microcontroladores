#include "TEA5767.h"
#include "I2C_Master.h"

#define TEA5767_ADDRESS 0x60

void TEA5767_Inicializar(void) {
    I2C_START();
    I2C_TRANSMITE(TEA5767_ADDRESS << 1);
    I2C_TRANSMITE(0x00); // Mute right and left audio
    I2C_TRANSMITE(0x00); // Frequency high byte
    I2C_TRANSMITE(0xB0); // Frequency low byte
    I2C_TRANSMITE(0x10); // Xtal frequency 32.768 kHz
    I2C_TRANSMITE(0x00); // No software mute, high side LO injection
    I2C_STOP();
}

void TEA5767_Sintonizar(float frequencia) {
    unsigned int freqB = (unsigned int)(4 * (frequencia * 1000000 + 225000) / 32768);
    unsigned char freqH = (freqB >> 8) & 0x3F; // 6 bits mais significativos
    unsigned char freqL = freqB & 0xFF;       // 8 bits menos significativos

    I2C_START();
    I2C_TRANSMITE(TEA5767_ADDRESS << 1);
    I2C_TRANSMITE(freqH); // Frequency high byte
    I2C_TRANSMITE(freqL); // Frequency low byte
    I2C_TRANSMITE(0xB0);  // Mute right and left audio
    I2C_TRANSMITE(0x10);  // Xtal frequency 32.768 kHz
    I2C_TRANSMITE(0x00);  // No software mute, high side LO injection
    I2C_STOP();
}