/*
 * Sistema de Localização de Motos em Pátio usando WiFi Fingerprinting
 * 
 * Este código implementa uma solução IoT para rastreamento de motos em pátios
 * usando apenas um roteador WiFi e técnica de fingerprinting de sinal.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>  // Para comunicação MQTT
#include <time.h>          // Para obter timestamp preciso

// ================= CONFIGURAÇÕES DE REDE =================
const char* SSID_ROUTER = "Wokwi-GUEST"; // SSID do roteador principal
const char* PASSWORD = "";        // Senha da rede WiFi

// ================= CONFIGURAÇÕES MQTT ====================
const char* MQTT_SERVER = "broker.hivemq.com"; // Broker público para testes
const int MQTT_PORT = 1883;                    // Porta padrão MQTT
const char* MQTT_TOPIC = "patio/motos/localizacao"; // Tópico para publicação

// ================= CONFIGURAÇÕES NTP =====================
const char* NTP_SERVER = "pool.ntp.org";       // Servidor de tempo
const long GMT_OFFSET_SEC = -3 * 3600;         // Fuso horário -3h (Brasília)
const int DAYLIGHT_OFFSET_SEC = 0;             // Sem horário de verão

// ================= DIMENSÕES DO PÁTIO ===================
const float PATIO_WIDTH = 50.0;   // Largura total do pátio em metros
const float PATIO_HEIGHT = 30.0;  // Altura total do pátio em metros

// ================= ESTRUTURAS DE DADOS ==================
// Armazena posições 2D (x,y)
struct Position {
  float x;  // Coordenada horizontal (metros)
  float y;  // Coordenada vertical (metros)
};

// Armazena os dados de fingerprint (mapeamento prévio)
struct Fingerprint {
  float x, y;  // Posição física no pátio
  int rssi;    // Valor médio de RSSI medido nesta posição
};

// ================= DATABASE DE FINGERPRINTS ==============
// Valores pré-mapeados (deve ser calibrado no ambiente real)
const Fingerprint fingerprintDB[5] = {
  // Centro do pátio
  {0.0f, 0.0f, -40},  // Posição (x,y) e RSSI médio
  
  // Cantos do pátio (inferior esquerdo, inferior direito, superior esquerdo, superior direito)
  {-PATIO_WIDTH/2, -PATIO_HEIGHT/2, -72},
  {PATIO_WIDTH/2, -PATIO_HEIGHT/2, -68},
  {-PATIO_WIDTH/2, PATIO_HEIGHT/2, -75}, 
  {PATIO_WIDTH/2, PATIO_HEIGHT/2, -70}
};

// ================= VARIÁVEIS GLOBAIS =====================
WiFiClient espClient;             // Cliente WiFi
PubSubClient mqttClient(espClient); // Cliente MQTT
Position currentEstimate = {0, 0}; // Posição atual estimada

// ================= FUNÇÕES AUXILIARES ====================

/**
 * Obtém o timestamp atual formatado
 * Retorna: String no formato "DD-MM-AAAA HH:MM:SS"
 */
String getFormattedTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){  // Tenta obter o tempo local
    return "00-00-0000 00:00:00"; // Retorno padrão em caso de falha
  }
  
  char timeString[20];
  // Formata a data e hora
  strftime(timeString, sizeof(timeString), "%d-%m-%Y %H:%M:%S", &timeinfo);
  return String(timeString);
}

/**
 * Configura a conexão WiFi e sincroniza com servidor NTP
 */
void setupWiFi() {
  Serial.begin(115200); // Inicia comunicação serial
  
  Serial.print("Conectando a ");
  Serial.println(SSID_ROUTER);

  WiFi.begin(SSID_ROUTER, PASSWORD); // Inicia conexão WiFi

  // Aguarda conexão ser estabelecida
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());

  // Configura o cliente NTP
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
}

/**
 * Reconecta ao broker MQTT em caso de desconexão
 */
void reconnectMQTT() {
  // Loop até conseguir reconectar
  while (!mqttClient.connected()) {
    Serial.print("Conectando ao MQTT...");
    
    // Tenta conectar com ID único
    if (mqttClient.connect("motoTracker01")) {
      Serial.println("Conectado!");
    } else {
      // Mostra erro e tenta novamente após 5s
      Serial.print("Falha, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Tentando novamente em 5s...");
      delay(5000);
    }
  }
}

/**
 * Estima a posição usando interpolação ponderada entre os fingerprints
 */
void estimatePosition() {
  int currentRSSI = WiFi.RSSI(); // Obtém intensidade do sinal atual
  float totalWeight = 0.0f;      // Soma total dos pesos
  float x = 0.0f, y = 0.0f;     // Acumuladores de posição

  // Calcula a contribuição de cada ponto de referência
  for (int i = 0; i < 5; i++) {
    // O peso é inversamente proporcional à diferença de RSSI
    float weight = 1.0f / (abs(fingerprintDB[i].rssi - currentRSSI) + 0.1f);
    totalWeight += weight;
    x += fingerprintDB[i].x * weight; // Acumula posição x ponderada
    y += fingerprintDB[i].y * weight; // Acumula posição y ponderada
  }

  // Calcula a posição média ponderada
  currentEstimate.x = x / totalWeight;
  currentEstimate.y = y / totalWeight;
}

/**
 * Envia os dados de localização via MQTT
 */
void sendLocation() {
  estimatePosition(); // Atualiza a estimativa de posição
  
  // Constrói o payload JSON
  String payload = "{";
  payload += "\"device_id\":\"moto_01\",";       // Identificação do dispositivo
  payload += "\"position\":{\"x\":" + String(currentEstimate.x, 2) + ",\"y\":" + String(currentEstimate.y, 2) + "},"; // Coordenadas
  payload += "\"rssi\":" + String(WiFi.RSSI()) + ",";  // Intensidade do sinal
  payload += "\"timestamp\":\"" + getFormattedTime() + "\","; // Data/hora
  payload += "\"bateria\":" + String(random(20, 101)); // Nível de bateria simulado
  payload += "}";

  // Publica no tópico MQTT
  if (mqttClient.publish(MQTT_TOPIC, payload.c_str())) {
    Serial.println("Dados enviados: " + payload);
  } else {
    Serial.println("Falha no envio MQTT");
  }
}

// ================= SETUP E LOOP PRINCIPAL ================

void setup() {
  setupWiFi(); // Configura WiFi e NTP
  
  // Configura cliente MQTT
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  
  // Inicializa gerador de números aleatórios
  randomSeed(analogRead(0)); 
}

void loop() {
  // Verifica conexão MQTT
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop(); // Mantém a conexão MQTT ativa
  
  sendLocation();    // Envia dados de localização
  delay(5000);       // Intervalo entre envios (5 segundos)
}