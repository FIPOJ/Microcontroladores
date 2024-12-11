#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define LED_VERDE 2
#define LED_VERMELHO 3
#define LED_RGB_R 4
#define LED_RGB_G 5
#define LED_RGB_B 6
#define BUZZER 7

#define BOTAO_LIGA_DESLIGA 8
#define BOTAO_AUMENTA_FREQUENCIA 9
#define BOTAO_DIMINUI_FREQUENCIA 10
#define BOTAO_AUMENTA_VOLUME 11
#define BOTAO_DIMINUI_VOLUME 12
#define BOTAO_FAVORITO1 13
#define BOTAO_FAVORITO2 A0

#define TEA5767_ADDRESS 0x60

LiquidCrystal_I2C lcd(0x27, 16, 2);
bool radioLigado = false;
bool pressedRB0 = false;
bool pressedRB1 = false;
bool pressedRB2 = false;
bool pressedRB3 = false;
bool pressedRB4 = false;
bool pressedRB5 = false;
bool pressedRB6 = false;
float frequenciaAtual = 102.1;
float favorito1 = 0;
float favorito2 = 0;
unsigned long pressStartTime = 0;
byte qualidadeSinal;


void setup() {
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_RGB_R, OUTPUT);
  pinMode(LED_RGB_G, OUTPUT);
  pinMode(LED_RGB_B, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  pinMode(BOTAO_LIGA_DESLIGA, INPUT_PULLUP);
  pinMode(BOTAO_AUMENTA_FREQUENCIA, INPUT_PULLUP);
  pinMode(BOTAO_DIMINUI_FREQUENCIA, INPUT_PULLUP);
  pinMode(BOTAO_AUMENTA_VOLUME, INPUT_PULLUP);
  pinMode(BOTAO_DIMINUI_VOLUME, INPUT_PULLUP);
  pinMode(BOTAO_FAVORITO1, INPUT_PULLUP);
  pinMode(BOTAO_FAVORITO2, INPUT_PULLUP);

  Wire.begin();
  lcd.init();
  lcd.backlight();
  inicializarTEA5767();
  desligarRadio();
}

void loop() {
  verificarBotaoLigaDesliga();
  if (radioLigado) {
    verificarBotoes();
  }
}

void verificarBotaoLigaDesliga() {
  if (digitalRead(BOTAO_LIGA_DESLIGA) == LOW && !pressedRB0) {
    delay(20);
    if (digitalRead(BOTAO_LIGA_DESLIGA) == LOW) {
      if (radioLigado) {
        desligarRadio();
      } else {
        ligarRadio();
      }
      pressedRB0 = true;
    }
  } else if (digitalRead(BOTAO_LIGA_DESLIGA) == HIGH) {
    pressedRB0 = false;
  }
}

void verificarBotoes() {
  if (digitalRead(BOTAO_AUMENTA_FREQUENCIA) == LOW && !pressedRB1) {
    delay(20);
    if (digitalRead(BOTAO_AUMENTA_FREQUENCIA) == LOW) {
      buscarProximaFrequencia(true);
      pressedRB1 = true;
    }
  } else if (digitalRead(BOTAO_AUMENTA_FREQUENCIA) == HIGH) {
    pressedRB1 = false;
  }

  if (digitalRead(BOTAO_DIMINUI_FREQUENCIA) == LOW && !pressedRB2) {
    delay(20);
    if (digitalRead(BOTAO_DIMINUI_FREQUENCIA) == LOW) {
      buscarProximaFrequencia(false);
      pressedRB2 = true;
    }
  } else if (digitalRead(BOTAO_DIMINUI_FREQUENCIA) == HIGH) {
    pressedRB2 = false;
  }

  if (digitalRead(BOTAO_AUMENTA_VOLUME) == LOW && !pressedRB3) {
    delay(20);
    if (digitalRead(BOTAO_AUMENTA_VOLUME) == LOW) {
      ajustarVolume(true);
      pressedRB3 = true;
    }
  } else if (digitalRead(BOTAO_AUMENTA_VOLUME) == HIGH) {
    pressedRB3 = false;
  }

  if (digitalRead(BOTAO_DIMINUI_VOLUME) == LOW && !pressedRB4) {
    delay(20);
    if (digitalRead(BOTAO_DIMINUI_VOLUME) == LOW) {
      ajustarVolume(false);
      pressedRB4 = true;
    }
  } else if (digitalRead(BOTAO_DIMINUI_VOLUME) == HIGH) {
    pressedRB4 = false;
  }

  verificarFavorito(BOTAO_FAVORITO1, &favorito1, &pressedRB5);
  verificarFavorito(BOTAO_FAVORITO2, &favorito2, &pressedRB6);
}

void inicializarTEA5767() {
  // Inicialização do TEA5767
  Wire.beginTransmission(TEA5767_ADDRESS);
  Wire.write((byte)0x00); // Configuração inicial
  Wire.write((byte)0x00);
  Wire.write((byte)0xB0);
  Wire.write((byte)0x10);
  Wire.write((byte)0x00);
  Wire.endTransmission();
}

void sintonizarTEA5767(float frequencia) {
  // Sintoniza o TEA5767 na frequência desejada
  uint16_t freqB = (uint16_t)(4 * (frequencia * 1000000 + 225000) / 32768);
  byte freqH = (freqB >> 8);
  byte freqL = (freqB & 0xFF);

  Wire.beginTransmission(TEA5767_ADDRESS);
  Wire.write(freqH);
  Wire.write(freqL);
  Wire.write(0xB0);
  Wire.write(0x10);
  Wire.write(0x00);
  Wire.endTransmission();
}

void ligarRadio() {
  radioLigado = true;
  digitalWrite(LED_VERDE, HIGH);
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);
  lcd.clear();
  atualizarLCD();
  sintonizarTEA5767(frequenciaAtual);
}

void desligarTEA5767() {
  // Envia um comando para colocar o TEA5767 em modo idle ou desligado
  Wire.beginTransmission(TEA5767_ADDRESS);
  Wire.write((byte)0x00); // Comando de desligamento ou idle
  Wire.write((byte)0x00);
  Wire.write((byte)0x00);
  Wire.write((byte)0x00);
  Wire.write((byte)0x00);
  Wire.endTransmission();
}

void desligarRadio() {
  radioLigado = false;
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, HIGH);
  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Desligado");
  desligarTEA5767(); // Chama a função para desligar o TEA5767
}


void buscarProximaFrequencia(bool incremento) {
  lcd.clear();
  while (true) {
    if (incremento) {
      frequenciaAtual += 0.1;
      if (frequenciaAtual > 108.0) {
        frequenciaAtual = 87.5;
      }
    } else {
      frequenciaAtual -= 0.1;
      if (frequenciaAtual < 87.5) {
        frequenciaAtual = 108.0;
      }
    }
    sintonizarTEA5767(frequenciaAtual);
    delay(500); // Aguarda um pouco para sintonizar

    // Verifica a qualidade do sinal
    if (verificarQualidadeSinal()) {
      break;
    }
  }

  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);
  atualizarLCD();
}

void ajustarVolume(bool incremento) {
  // Ajustar o volume (TEA5767 não tem controle de volume por I2C)
  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);
}

void salvarFavorito(unsigned char favorito) {
  // Salvar a frequência atual como favorita
  if (favorito == 1) {
    favorito1 = frequenciaAtual;
  } else if (favorito == 2) {
    favorito2 = frequenciaAtual;
  }
  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);
}

void atualizarLCD() {
  lcd.setCursor(0, 0);
  lcd.print("Freq:");
  lcd.print(frequenciaAtual, 1);
  lcd.setCursor(0, 1);
  lcd.print("Sinal:");
  lcd.print(qualidadeSinal);
}


bool verificarQualidadeSinal() {
  // Leitura dos dados do TEA5767 para verificar a qualidade do sinal
  Wire.requestFrom(TEA5767_ADDRESS, 5);
  qualidadeSinal = 0;
  if (Wire.available() == 5) {
    Wire.read(); // Leitura do byte 1 (ignorado)
    Wire.read(); // Leitura do byte 2 (ignorado)
    qualidadeSinal = Wire.read(); // Leitura do byte 3 (qualidade do sinal)
    Wire.read(); // Leitura do byte 4 (ignorado)
    Wire.read(); // Leitura do byte 5 (ignorado)
  }

  // Ajusta o LED RGB com base na qualidade do sinal
  if (qualidadeSinal < 61) {  // Sinal ruim
    digitalWrite(LED_RGB_R, HIGH);
    digitalWrite(LED_RGB_G, HIGH);
    digitalWrite(LED_RGB_B, LOW);
    return false;
  } else if (qualidadeSinal < 70) {  // Sinal fraco
    digitalWrite(LED_RGB_R, HIGH);
    digitalWrite(LED_RGB_G, HIGH);
    digitalWrite(LED_RGB_B, LOW);
    return true;
  } else if (qualidadeSinal < 130) {  // Sinal médio
    digitalWrite(LED_RGB_R, HIGH);
    digitalWrite(LED_RGB_G, HIGH);
    digitalWrite(LED_RGB_B, LOW);
    return true;
  } else {  // Sinal bom
    digitalWrite(LED_RGB_R, LOW);
    digitalWrite(LED_RGB_G, HIGH);
    digitalWrite(LED_RGB_B, LOW);
    return true;
  }
}


void verificarFavorito(int botao, float* favorito, bool* pressed) {
  if (digitalRead(botao) == LOW) {
    if (!*pressed) {
      pressStartTime = millis();
      *pressed = true;
    } else if (millis() - pressStartTime >= 3000) {
      // Se o botão foi pressionado por 3 segundos, salva a frequência atual
      salvarFavorito(botao == BOTAO_FAVORITO1 ? 1 : 2);
      *pressed = false;
    }
  } else {
    if (*pressed) {
      if (millis() - pressStartTime < 3000) {
        // Se o botão foi pressionado por menos de 3 segundos, sintoniza na frequência favorita
        if (*favorito > 0) {
          frequenciaAtual = *favorito;
          sintonizarTEA5767(frequenciaAtual);
          atualizarLCD();
        }
      }
      *pressed = false;
    }
  }
}
