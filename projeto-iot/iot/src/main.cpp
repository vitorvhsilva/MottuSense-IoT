#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Configurações WiFi
const char* SSID_ROUTER = "PatioTracker";
const char* PASSWORD = "senha_segura";

// Configurações MQTT
const char* MQTT_SERVER = "broker.hivemq.com"; // Broker público para teste
const int MQTT_PORT = 1883;
const char* MQTT_TOPIC = "patio/motos/tracking";

// Hardware
#define LED_PIN 2 // LED interno da ESP32
bool SIMULATION_MODE = true; // Altere para false no dispositivo real

WiFiClient espClient;
PubSubClient client(espClient);

// Variáveis de simulação
int simulatedDistance = 5; // metros
bool increasingDistance = true;
unsigned long lastUpdate = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(SSID_ROUTER);

  WiFi.begin(SSID_ROUTER, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }

  digitalWrite(LED_PIN, HIGH);
  Serial.println("\nWiFi conectado");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    
    if (client.connect("motoTrackerSimulator")) {
      Serial.println("Conectado");
      digitalWrite(LED_PIN, HIGH);
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5s");
      digitalWrite(LED_PIN, LOW);
      delay(5000);
    }
  }
}

int simulate_rssi() {
  // Simula movimento entre 2m e 20m do roteador
  if (millis() - lastUpdate > 3000) {
    lastUpdate = millis();
    
    if (increasingDistance) {
      simulatedDistance += 1;
      if (simulatedDistance >= 20) increasingDistance = false;
    } else {
      simulatedDistance -= 1;
      if (simulatedDistance <= 2) increasingDistance = true;
    }
  }

  // Modelo de propagação
  float measured_power = -40.0; // RSSI a 1 metro
  float path_loss_exponent = 3.5;
  return (int)(measured_power - 10 * path_loss_exponent * log10(simulatedDistance));
}

void send_location() {
  int rssi = SIMULATION_MODE ? simulate_rssi() : WiFi.RSSI();
  float distance = pow(10, (-40.0 - rssi) / (10 * 3.5));
  
  // Prepara payload JSON
  String payload = "{";
  payload += "\"device\":\"moto_simulator\",";
  payload += "\"rssi\":" + String(rssi) + ",";
  payload += "\"distance\":" + String(distance, 2) + ",";
  payload += "\"simulated\":" + String(SIMULATION_MODE ? "true" : "false");
  payload += "}";

  // Publica no MQTT
  client.publish(MQTT_TOPIC, payload.c_str());
  Serial.println("Dados enviados: " + payload);
  
  // Feedback visual pelo LED
  digitalWrite(LED_PIN, LOW);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  
  if(SIMULATION_MODE) {
    Serial.println("Modo Simulação Ativado");
  }
}

void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();
  
  send_location();
  delay(5000); // Envia a cada 5 segundos
}