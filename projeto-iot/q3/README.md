## Questão 3 – Supervisão e Automação com Atuadores Inteligentes

### Objetivo do Desafio

Transformar um código de demonstração de atuadores em um sistema inteligente controlado remotamente por mensagens MQTT. O objetivo é permitir que um painel supervisório (como o Node-RED) envie comandos para o ESP32, ativando ou desativando os atuadores de forma individual ou em conjunto, como se fosse um painel de controle remoto via nuvem.

### O que já está pronto

Um código funcional que:

- Aciona um buzzer com frequência de 1 kHz.

- Liga/desliga uma luz comum (LED simples).

- Realiza um ciclo de cores no LED RGB (vermelho, verde, azul).

- Move um servo motor entre as posições 0°, 90° e 180°.

`Esse código é uma demo local automática e não possui conectividade. Sua missão é transformar isso em um dispositivo IoT de verdade!`

### Sua Tarefa

Modificar o código para torná-lo conectado com MQTT, seguindo o modelo da Questão 1:

- Conecte o ESP32 ao Wi-Fi.
- Configure um broker MQTT.

Inscreva-se em tópicos para receber comandos de controle dos atuadores:

- Crie tópicos padronizados, únicos e personalizados para cada comando 
- Interpretar mensagens em JSON para ativar ou desativar cada atuador individualmente. 

Criar um painel no Node-RED que permita:

- Enviar comandos para os atuadores via botões, sliders ou dropdowns.
- Visualizar em tempo real quais atuadores estão ativos.

- (Opcional) Criar automações simples, como "ligar o buzzer se o LED RGB estiver vermelho".

### Dicas

- Reutilize o código MQTT da Questão 1 como base.
- Dimensione corretamente o StaticJsonDocument para processar comandos JSON recebidos.

- Para representar cores, crie uma função como:
```cpp
void setRGB(String cor);
```

- Você pode deixar os atuadores desligados até que um comando MQTT seja recebido.

