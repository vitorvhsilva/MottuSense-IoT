#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <math.h>
#include <time.h>

// 1. Configurações de Rede
const char* SSID_ROUTER = "Wokwi-GUEST";
const char* PASSWORD = "";

// 2. Configurações MQTT
const char* MQTT_SERVER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char* MQTT_TOPIC = "patio/motos/tracking";

// 3. Configurações NTP
const char* NTP_SERVER = "pool.ntp.org";
const long  GMT_OFFSET_SEC = -3 * 3600; // Fuso horário -3h (Brasília)
const int   DAYLIGHT_OFFSET_SEC = 0;    // Sem horário de verão

// 4. Dimensões do Pátio (metros)
const float PATIO_WIDTH = 50.0;
const float PATIO_HEIGHT = 30.0;

// 5. Posições dos 5 roteadores (x, y)
const float routerPositions[5][2] = {
  {0, 0},                   // Roteador 0: Centro
  {-PATIO_WIDTH/2, -PATIO_HEIGHT/2},  // Roteador 1: Canto inferior esquerdo
  {PATIO_WIDTH/2, -PATIO_HEIGHT/2},   // Roteador 2: Canto inferior direito
  {-PATIO_WIDTH/2, PATIO_HEIGHT/2},   // Roteador 3: Canto superior esquerdo
  {PATIO_WIDTH/2, PATIO_HEIGHT/2}     // Roteador 4: Canto superior direito
};

// 6. Hardware
#define LED_PIN 2

// 7. Variáveis globais
WiFiClient espClient;
PubSubClient client(espClient);

struct Position {
  float x;
  float y;
};

Position currentPosition = {0, 0};
unsigned long lastUpdate = 0;
float pathLossExponent = 3.5;
float measuredPower = -40.0;

// 8. Função para obter timestamp formatado
String getFormattedTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    return "00-00-0000 00:00:00";
  }
  
  char timeString[20];
  strftime(timeString, sizeof(timeString), "%d-%m-%Y %H:%M:%S", &timeinfo);
  return String(timeString);
}

// 9. Função de conexão WiFi
void setup_wifi() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  
  Serial.print("Conectando a ");
  Serial.println(SSID_ROUTER);

  WiFi.begin(SSID_ROUTER, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }

  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Configura o servidor NTP
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
}

// 10. Conexão MQTT
void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Conectando MQTT...");
    if (client.connect("motoTracker5AP")) {
      Serial.println("Conectado");
    } else {
      Serial.print("falha, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

// 11. Simulação de distâncias para cada roteador
void simulateRoutersRSSI(float distances[5]) {
  if (millis() - lastUpdate > 3000) {
    lastUpdate = millis();
    
    currentPosition.x += random(-200, 201)/100.0;
    currentPosition.y += random(-200, 201)/100.0;
    
    currentPosition.x = constrain(currentPosition.x, -PATIO_WIDTH/2, PATIO_WIDTH/2);
    currentPosition.y = constrain(currentPosition.y, -PATIO_HEIGHT/2, PATIO_HEIGHT/2);
  }
  
  for (int i = 0; i < 5; i++) {
    float dx = currentPosition.x - routerPositions[i][0];
    float dy = currentPosition.y - routerPositions[i][1];
    distances[i] = sqrt(dx*dx + dy*dy);
  }
}

// 12. Trilateração com 5 pontos usando mínimos quadrados
Position calculatePosition(float distances[5]) {
  float A[4][2], B[4];
  
  for (int i = 0; i < 4; i++) {
    A[i][0] = 2*(routerPositions[i+1][0] - routerPositions[0][0]);
    A[i][1] = 2*(routerPositions[i+1][1] - routerPositions[0][1]);
    B[i] = pow(distances[0], 2) - pow(distances[i+1], 2)
      - pow(routerPositions[0][0], 2) + pow(routerPositions[i+1][0], 2)
      - pow(routerPositions[0][1], 2) + pow(routerPositions[i+1][1], 2);
  }

  float ata[2][2] = {0}, atb[2] = {0};
  
  for (int i = 0; i < 4; i++) {
    ata[0][0] += A[i][0] * A[i][0];
    ata[0][1] += A[i][0] * A[i][1];
    ata[1][0] += A[i][1] * A[i][0];
    ata[1][1] += A[i][1] * A[i][1];
    
    atb[0] += A[i][0] * B[i];
    atb[1] += A[i][1] * B[i];
  }

  float det = ata[0][0]*ata[1][1] - ata[0][1]*ata[1][0];
  
  Position pos;
  if (fabs(det) > 0.001) {
    pos.x = (ata[1][1]*atb[0] - ata[0][1]*atb[1]) / det;
    pos.y = (ata[0][0]*atb[1] - ata[1][0]*atb[0]) / det;
  } else {
    pos = currentPosition;
  }
  
  return pos;
}

// 13. Envio de dados via MQTT
void sendLocation() {
  float distances[5];
  simulateRoutersRSSI(distances);
  
  Position newPos = calculatePosition(distances);
  
  if (!isnan(newPos.x) && !isnan(newPos.y)) {
    currentPosition = newPos;
  }

  String payload = "{";
  payload += "\"position\":{\"x\":" + String(currentPosition.x, 2) + ",\"y\":" + String(currentPosition.y, 2) + "},";
  payload += "\"distances\":[";
  for (int i = 0; i < 5; i++) {
    if (i > 0) payload += ",";
    payload += String(distances[i], 2);
  }
  payload += "],\"timestamp\":\"" + getFormattedTime() + "\"}";

  client.publish(MQTT_TOPIC, payload.c_str());
  Serial.println(payload);
}

// 14. Setup inicial
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  randomSeed(analogRead(0));
  
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
}

// 15. Loop principal
void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();
  
  sendLocation();
  delay(5000);
}