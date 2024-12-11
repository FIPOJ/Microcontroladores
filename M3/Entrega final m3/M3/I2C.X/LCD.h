#ifndef LCD_H
#define LCD_H

void LCD_Inicializar(void);
void LCD_Limpar(void);
void LCD_DefinirCursor(unsigned char linha, unsigned char coluna);
void LCD_EscreverString(char* str);
void LCD_EscreverFloat(float valor, unsigned char casasDecimais);
void LCD_EscreverNumero(int numero);
void I2C_EnviarNibble(unsigned char nibble);

#endif