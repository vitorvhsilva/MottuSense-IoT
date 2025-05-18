#include <Arduino.h>
#include <ESP32Servo.h>

// Definições de pinos
const int SERVO_PIN = 27;
const int BUZZER_PIN = 13;
const int LIGHT_PIN  = 15;

const int LED_R = 21;
const int LED_G = 22;
const int LED_B = 23;


Servo myservo;

void setup() {
  Serial.begin(9600);
  // Configuração dos pinos como saída
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  // Inicializa servo com limites adequados para ESP32
  myservo.attach(SERVO_PIN, 500, 2400);

  Serial.println("Demo de atuadores iniciada!");
}

void loop() {
  // 1. Teste do buzzer
  Serial.println("Ativando buzzer");
  tone(BUZZER_PIN, 1000); // 1000 Hz
  delay(1000);
  noTone(BUZZER_PIN);
  delay(500);

  // 2. Teste da luz comum
  Serial.println("Ativando luz");
  digitalWrite(LIGHT_PIN, HIGH);
  delay(1000);
  digitalWrite(LIGHT_PIN, LOW);
  delay(500);

  // 3. Teste dos LEDs RGB
  Serial.println("Ciclo RGB");
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);
  delay(500);

  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, HIGH);
  delay(500);

  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, HIGH);
  delay(500);

  digitalWrite(LED_B, LOW); // Desliga todos
  delay(500);

  // 4. Movimento do Servo
  Serial.println("Movendo servo de 0 a 180 graus");
  myservo.write(0);
  delay(2000);
  myservo.write(90);
  delay(2000);
  myservo.write(180);
  delay(2000);

  Serial.println("Ciclo completo. Aguardando 2 segundos...");
  delay(2000);
}
