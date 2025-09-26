/*
 * Sistema de Localização de Motos em Pátio usando WiFi Fingerprinting
 * Versão otimizada com gerenciamento robusto de conexões
 */
 
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <ThingSpeak.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
 
// ================= CONFIGURAÇÕES DE REDE =================
const char* SSID_ROUTER = "Wokwi-GUEST";
const char* PASSWORD = "";
 
// ================= CONFIGURAÇÕES MQTT ====================
const char* MQTT_SERVER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char* MQTT_TOPIC = "vitorvhsilva/patio/motos/localizacao";
const char* MQTT_CLIENT_ID = "motoTracker01"; // ID único para o cliente MQTT
 
// ================= CONFIGURAÇÕES NTP =====================
const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SEC = -3 * 3600;
const int DAYLIGHT_OFFSET_SEC = 0;
 
// ================= CONFIGURAÇÕES DO SISTEMA ==============
const float PATIO_WIDTH = 50.0;
const float PATIO_HEIGHT = 30.0;
const unsigned long PUBLISH_INTERVAL = 20000; // Intervalo de publicação em ms
 
// ================= ESTRUTURAS E VARIÁVEIS ================
struct Position { float x, y; };
struct Fingerprint { float x, y; int rssi; };
 
const Fingerprint fingerprintDB[5] = {
  {0.0f, 0.0f, -40},
  {-PATIO_WIDTH/2, -PATIO_HEIGHT/2, -72},
  {PATIO_WIDTH/2, -PATIO_HEIGHT/2, -68},
  {-PATIO_WIDTH/2, PATIO_HEIGHT/2, -75},
  {PATIO_WIDTH/2, PATIO_HEIGHT/2, -70}
};
 
// ================= OBJETOS GLOBAIS =======================
WiFiClient espClient;
PubSubClient mqttClient(espClient);
Position currentEstimate = {0, 0};
 
// ================= CONFIGURAÇÕES THINGSPEAK ==============
unsigned long channelID = 2969337;
const char* writeAPIKey = "L4A6INVG5ESYJI35";
 
// ================= PROTÓTIPOS DE FUNÇÃO ==================
const char* EVENTOS_URL = "https://mottusense-dotnet.onrender.com/api/v1/eventos";
 
String toIsoLocalWithOffset(time_t tt);
void sendEvent();
void setupWiFi();
void initMQTT();
void reconnectMQTT();
void maintainConnections();
String getFormattedTime();
void estimatePosition();
void sendLocation();

 
String toIsoLocalWithOffset(time_t tt) {
  struct tm tmLocal;
  localtime_r(&tt, &tmLocal);

  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S%z", &tmLocal);

  String s(buf);
  if (s.length() > 22) {
    s = s.substring(0, 22) + ":" + s.substring(22);
  }
  return s;
}
 
void sendEvent() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Evento] Sem WiFi");
    return;
  }
 
  time_t agora;
  time(&agora);
 
  StaticJsonDocument<192> doc;
  doc["IdMoto"] = "1fe80cb9-72de-4a9c-941e-da2b28cd8737";
  doc["IdEvento"] = 1;
  doc["DataHoraEvento"] = toIsoLocalWithOffset(agora);
 
  String payload;
  serializeJson(doc, payload);
 
  WiFiClientSecure client;
  client.setInsecure();
 
  HTTPClient http;
  http.begin(EVENTOS_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
 
  int code = http.POST(payload);
  Serial.printf("[Evento] POST %s -> %d\n", EVENTOS_URL, code);
  if (code > 0) {
    String resp = http.getString();
    Serial.println("[Evento] Resposta:");
    Serial.println(resp);
  } else {
    Serial.printf("[Evento] Erro HTTPClient: %d\n", code);
  }
  http.end();
}
 
void setupWiFi() {
  Serial.print("Conectando a ");
  WiFi.begin(SSID_ROUTER, PASSWORD);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("\nConectado! IP: " + WiFi.localIP().toString());
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
}
 
void initMQTT() {
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
}
 
void maintainConnections() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconectando WiFi...");
    setupWiFi();
  }
 
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
}
 
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Conectando ao MQTT...");
   
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("Conectado!");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Tentando novamente em 5s...");
      delay(5000);
    }
  }
}
 
String getFormattedTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return "00-00-0000 00:00:00";
 
  char timeString[20];
  strftime(timeString, sizeof(timeString), "%d-%m-%Y %H:%M:%S", &timeinfo);
  return String(timeString);
}
 
void estimatePosition() {
  int currentRSSI = WiFi.RSSI();
  float totalWeight = 0.0f, x = 0.0f, y = 0.0f;
 
  for (int i = 0; i < 5; i++) {
    float weight = 1.0f / (abs(fingerprintDB[i].rssi - currentRSSI) + 0.1f);
    totalWeight += weight;
    x += fingerprintDB[i].x * weight;
    y += fingerprintDB[i].y * weight;
  }
 
  currentEstimate.x = x / totalWeight;
  currentEstimate.y = y / totalWeight;
}
 
void sendLocation() {
  estimatePosition();
 
  // Preparar payload JSON
  StaticJsonDocument<256> doc;
  doc["device_id"] = "moto_01";
  doc["position"]["x"] = currentEstimate.x;
  doc["position"]["y"] = currentEstimate.y;
  doc["rssi"] = WiFi.RSSI();
  doc["timestamp"] = getFormattedTime();
 
  char payload[256];
  serializeJson(doc, payload);
 
  // Publicar no MQTT
  if (mqttClient.publish(MQTT_TOPIC, payload)) {
    Serial.println("MQTT enviado: " + String(payload));
  } else {
    Serial.println("Falha no envio MQTT!");
  }
 
  // Enviar para ThingSpeak
  ThingSpeak.setField(1, currentEstimate.x);
  ThingSpeak.setField(2, currentEstimate.y);
  ThingSpeak.setField(3, WiFi.RSSI());
 
  int status = ThingSpeak.writeFields(channelID, writeAPIKey);
  if (status == 200) {
    Serial.println("Dados enviados para ThingSpeak!");
    sendEvent();
  } else {
    Serial.println("Erro ThingSpeak: " + String(status));
  }
}
 
void setup() {
  Serial.begin(115200);
  setupWiFi();
  ThingSpeak.begin(espClient);
  initMQTT();
  randomSeed(analogRead(0));
}
 
void loop() {
  maintainConnections();
  mqttClient.loop();
  static unsigned long lastSend = 0;
 
  if (millis() - lastSend >= PUBLISH_INTERVAL) {
    sendLocation();
    lastSend = millis();
  }
 
  delay(20000);
}
 
 
 