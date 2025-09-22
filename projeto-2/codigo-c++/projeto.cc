#include <LiquidCrystal.h> 

// CONFIGURAÇÃO DE TESTE OU REAL
#define MODO_TESTE false   // troque para false quando quiser 1h real

// Define os pinos do LCD
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Variáveis de ambiente
const int pinoSensorUmidade = A0;
const int pinoSensorTemp = A1;
const int pinoChuva = A2;   // botão simulando sensor de chuva
const int pinoVento = A3;   // potenciômetro simulando vento
const int pinoValvula = 10;

const int VALVULA_LIGADA = LOW;
const int VALVULA_DESLIGADA = HIGH;

const int limiarSeco = 40;    // se <= 40% = solo seco - rega
const int tempoRega = 5;      // tempo de rega em segundos

int umidadeSolo = 0;
float temperatura = 0.0;
int estadoChuva = 0;
int velocidadeVento = 0;

// Controle de rega por intervalo
unsigned long intervaloRegaMillis = MODO_TESTE ? 4500UL : 3600000UL; 
unsigned long ultimoMomentoRega = 0;      

// Controle da troca de telas
enum Tela { TELA1, TELA2, TELA3 };
Tela telaAtual = TELA1;
unsigned long tempoUltimaTrocaTela = 0;
const unsigned long duracaoTrocaTela = 3000; // 3 segundos

// Controle da tela 3
String mensagemTela3 = "";

// Controle da rega
bool regando = false;
unsigned long inicioRega = 0;

void setup() {
  pinMode(pinoValvula, OUTPUT);
  pinMode(pinoChuva, INPUT_PULLUP);
  digitalWrite(pinoValvula, VALVULA_DESLIGADA);

  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("    Estacao");
  lcd.setCursor(0, 1);
  lcd.print(" Meteorologica");

  Serial.begin(9600);
  ultimoMomentoRega = millis();
}

void loop() {
  unsigned long agora = millis();

  // Leitura dos sensores
  umidadeSolo = analogRead(pinoSensorUmidade);
  umidadeSolo = map(umidadeSolo, 1023, 0, 0, 100);

  int valorBruto = analogRead(pinoSensorTemp);
  float tensao = valorBruto * (5.0 / 1023.0);
  temperatura = (tensao - 0.5) * 100.0;

  estadoChuva = digitalRead(pinoChuva);
  int valorVentoBruto = analogRead(pinoVento);
  velocidadeVento = map(valorVentoBruto, 0, 1023, 0, 100);

  bool intervaloPassou = (agora - ultimoMomentoRega) >= intervaloRegaMillis;

  // Controle do motor
  bool estadoAntes = regando; // guardar estado anterior para detectar mudança

  if (regando && (agora - inicioRega >= (unsigned long)tempoRega * 1000UL)) {
    digitalWrite(pinoValvula, VALVULA_DESLIGADA); // desliga motor
    regando = false;
    Serial.println("Motor: DESLIGADO (tempo de rega atingido)");
  }

  if (!regando && umidadeSolo <= limiarSeco && estadoChuva != LOW && intervaloPassou) {
    digitalWrite(pinoValvula, VALVULA_LIGADA); // liga motor
    inicioRega = agora;
    regando = true;
    ultimoMomentoRega = agora;
    Serial.print("Motor: LIGADO (umid=");
    Serial.print(umidadeSolo);
    Serial.print("%, chuva=");
    Serial.print(estadoChuva==LOW ? "SIM":"NAO");
    Serial.println(")");
  }

  // Se o estado do motor mudou, atualiza o LCD imediatamente para refletir
  if (regando != estadoAntes) {
    // atualiza mensagem imediata
    if (regando) {
      mensagemTela3 = "Regando...";
      // mostrar imediatamente
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Regando...");
    } else {
      mensagemTela3 = "Fim da rega";
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Fim da rega");
    }
    // evita que a rotina normal de troca de telas sobrescreva imediatamente
    tempoUltimaTrocaTela = agora;
  }

  // Atualiza mensagem da tela 3
  if (!regando) {
    if (umidadeSolo > limiarSeco) {
      mensagemTela3 = "Solo Encharcado";
    } else if (estadoChuva == LOW) {
      mensagemTela3 = "Chuva - sem rega";
    } else {
      mensagemTela3 = "Intervalo";
    }
  }

  // Controle das telas
  if (agora - tempoUltimaTrocaTela >= duracaoTrocaTela) {
    tempoUltimaTrocaTela = agora;
    lcd.clear();

    switch (telaAtual) {
      case TELA1:
        lcd.setCursor(0, 0);
        lcd.print("Temp:");
        lcd.print(temperatura, 1);
        lcd.print("C");
        lcd.setCursor(0, 1);
        lcd.print("Vento:");
        lcd.print(velocidadeVento);
        lcd.print("km/h");
        telaAtual = TELA2;
        break;

      case TELA2:
        lcd.setCursor(0, 0);
        if (estadoChuva == LOW)
          lcd.print("Chovendo");
        else
          lcd.print("Sem chuva");
        lcd.setCursor(0, 1);
        lcd.print("Umid:");
        lcd.print(umidadeSolo);
        lcd.print("%");
        telaAtual = TELA3;
        break;

      case TELA3:
        lcd.setCursor(0, 0);
        lcd.print(mensagemTela3);
        telaAtual = TELA1;
        break;
    }
  }
  delay(10);
}
