# Sistema de Localização de Motos via WiFi Fingerprinting

## Visão Geral
Solução IoT para rastreamento preciso de motos em pátios utilizando apenas um roteador WiFi e técnica de fingerprinting de sinal. O sistema estima posições (x,y) com base em padrões de intensidade de sinal pré-mapeados.

## Tecnologias Utilizadas

### Hardware
- **ESP32**: Microcontrolador com WiFi integrado
- **Roteador WiFi**: Ponto de acesso central (2.4GHz recomendado)

### Software
- **PlatformIO**: Ambiente de desenvolvimento
- **Arduino Framework**: Programação do ESP32
- **Protocolo MQTT**: Comunicação com broker (HiveMQ público)
- **NTP**: Sincronização de tempo preciso

### Bibliotecas
- `WiFi.h`: Conexão WiFi
- `PubSubClient.h`: Comunicação MQTT
- `time.h`: Manipulação de timestamp

## Instruções de Uso

### Pré-requisitos
1. Mapear o pátio coletando RSSI nos pontos de referência:
```cpp
const Fingerprint fingerprintDB[5] = {
  {0.0f, 0.0f, -40},   // Centro
  {-25.0f, -15.0f, -72}, // Canto inf. esq.
  //... outros pontos
};
```
2. Editar constantes do código
```cpp
// Rede WiFi
const char* SSID_ROUTER = "SEU_SSID";
const char* PASSWORD = "SUA_SENHA";

// Dimensões do pátio (metros)
const float PATIO_WIDTH = 50.0;
const float PATIO_HEIGHT = 30.0;
```

## Instalação
1. Clonar repositório
```cpp
git clone https://github.com/seu-usuario/moto-tracking.git
```

2. Abrir no PlatformIO
```cpp
pio project init --ide vscode
```

3. Executar o Build do projeto e rodar o Workwi no diagram.json

## Exemplo de Saída
```bash
json
{
  "device_id": "moto_01",
  "position": {
    "x": -12.3,
    "y": 8.7
  },
  "rssi": -68,
  "timestamp": "15-06-2023 14:30:45"
}
```
