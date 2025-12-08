#include <Arduino.h>

// Definição dos pinos
#define PIN_GATE 3     // Digital 3 - Gate do Mosfet que vai acionar a carga. 
#define PIN_SHUNT A2   // Analogic 2 - Amplificador diferencial que vai entregar a tensão usada para medir a corrente
#define PIN_BUFFER A1  // Analogic 1 - OPAMP na topologia buffer apenas para medir a tensão da bateria no terminal

// Resistores - Ohms
float LOAD = 4.0;
float SHUNT = 0.1;
float DR1 = 10000.0;
float DR2 = 3000.0;
float R1 = 1000.0;
float R2 = 1000.0;
float R3 = 10000.0;
float R4 = 10000.0;

// Constantes
const float BK =  1.00081387;  // Adimensional - Correção de Offset e divisor de tensão. Medir empiricamente!
const float GK = 1.007814766;  // Adimensional -  Devido ao Offset e também da imprecisão dos resistores. Medir empiricamente!

const float ADCBITS = 1024.0;  // Definição da quantidade de bits para o cálculo do ADC! No proteus é usado 1024, na prática são 1023!
float VREF = 1.1;  // Volts - Tensão de referência - Utilize getVref(sinal) com um sinal conhecido no pino A0 para descobrir a referencia real
const float VMIN = 2.8;  // Volts - Tensão mínima para a bateria 18650! 
const float BETA = VREF / ADCBITS;  // Volts / Bits - Fator de multiplicação para a tensão
const float DIVISOR = (DR2 / (DR1 + DR2)) * BK;  // Adimensional - Fator do divisor de tensão
const float GANHO = (R4 / (R2+R4) ) * ((R1 + R3) / R1) * GK;   // Adimensional - Ganho do OPAMP

// Variáveis globais
float tensao;  // Volts - analogRead(PIN_BUFFER) * BETA / DIVISOR;
float corrente;  // AMPERES - analogRead(PIN_SHUNT) * BETA / GANHO / SHUNT;

// Componentes para a integração
float soma = 0;  // Variável que armazena o somatório
unsigned long x[2];  // Componente x da regra do trapézio (variação de tempo)
float y[2];  // Componente y da regra do trapézio (variação da corrente)

// Declaração das funções
void verificar();
void integrar();
float getVref();


void setup() {
  Serial.begin(9600);
  Serial.setTimeout(1000000);
  pinMode(PIN_GATE, OUTPUT);  
  analogReference(INTERNAL);
}


void loop(){
  delay(1000);
  digitalWrite(PIN_GATE, HIGH);

  //tensao = analogRead(A1) * BETA / DIVISOR;
  //Serial.println(tensao, 10);

  //corrente = analogRead(PIN_SHUNT) * BETA / GANHO / SHUNT;
  //Serial.println(corrente, 10);
  
  verificar();
}


void integrar(){
  // Descarrega a bateria e faz a contagem da carga
  Serial.println("Iniciando contagem!");
  Serial.println();
  digitalWrite(PIN_GATE, HIGH);
  delay(10);  // Delay pro circuito ajustar a descarga
  int counter = 1;  // Apenas para dar um feedback ao usuário

  x[0] = millis();
  y[0] = analogRead(PIN_SHUNT) * BETA / GANHO / SHUNT;
  
  while (tensao >= VMIN){
    tensao = analogRead(PIN_BUFFER) * BETA / DIVISOR;
    delayMicroseconds(100);
    corrente = analogRead(PIN_SHUNT) * BETA / GANHO / SHUNT;
    x[1] = millis();
    y[1] = corrente;

    soma += (y[0] + y[1]) * (x[1] - x[0]) / 7200;
    
    x[0] = x[1];
    y[0] = y[1];

    counter += 1;

    if (counter >= 10){
      Serial.print("Tensão: ");
      Serial.print(tensao, 6);
      Serial.println(" volts.\n");

      Serial.print("Corrente: ");
      Serial.print(corrente, 6);
      Serial.println(" amperes.\n");

      Serial.print("Capacidade: ");
      Serial.print(soma, 6);
      Serial.println(" mAh.\n");
      Serial.println();
      
      counter = 1;
    }

    delay(100);
  }

  Serial.print("Teste Concluído! Sua bateria tem: ");
  Serial.print(soma, 6);
  Serial.println("mAh. ");
  Serial.println("Pressione qualquer tecla para continuar.\n");
  Serial.readStringUntil('\r');
}


void verificar(){
  // Analisa a condição da bateria e determina o proximo passo
  tensao = analogRead(PIN_BUFFER) * BETA / DIVISOR;
  digitalWrite(PIN_GATE, LOW);

  if (tensao < 1) {
    Serial.println("Bateria invertida! Por favor, confira a polaridade.");
    Serial.println("Pressione 'enter' para verificar novamente.");
    Serial.readStringUntil('\r');
  }
  else if ((tensao >= 1) && (tensao <= 3.6)){
    Serial.println("Bateria não está completamente carregada! Tensão atual: " + String(tensao) + " volts.");
    Serial.println("Quando estiver carregada, pressione 'enter' para verificar novamente.");
    Serial.readStringUntil('\r');
  }
  else {
    Serial.println("Bateria carregada e pronta para o teste!");
    Serial.println("O teste pode demorar vários minutos, pressione 'enter' quando estiver pronto.");
    Serial.readStringUntil('\r');
    integrar();
  }
}


float getVref(float signal = 0.55){
  // Retorna o valor REAL da referencia interna! Usar para anotar a constante da placa! Use um SINAL conhecido!!!
    // Utilize um sinal EXTREMAMENTE preciso na porta A0!  
  float media = 0;
  for (int i = 0; i < 10; i++){
    float registro = analogRead(A0);
    media += registro;
    delayMicroseconds(100);
  }
  media /= 10;

  return (signal * ADCBITS / media);
}