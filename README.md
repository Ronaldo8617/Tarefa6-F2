# 🌊 Estação de Monitoramento de Cheias - Raspberry Pi Pico (BitDog Lab)

## 📌 Sumário  
- [📹 Demonstração](#-demonstração)  
- [🎯 Objetivo](#-objetivo)  
- [🛠️ Funcionalidades Obrigatórias](#️-funcionalidades-obrigatórias)  
- [📦 Componentes Utilizados](#-componentes-utilizados)  
- [⚙️ Compilação e Gravação](#️-compilação-e-gravação)  
- [📂 Estrutura do Código](#-estrutura-do-código)  
- [👨‍💻 Autor](#-autor)  

## 📹 Demonstração  
[clique aqui para acessar o vídeo](https://drive.google.com/file/d/1KGhIHI-ZHLxwJsvF2xodwm_9o59Hkp7k/view?usp=drive_link)
 
Conteúdo do vídeo:  
- Apresentação do projeto  
- Demonstração da leitura de sensores analógicos (chuva e nível) 
- Atuação em tempo real com LEDs RGB, buzzer, matriz WS2818B e OLED 


## 🎯 Objetivo  
Desenvolver uma estação de monitoramento de cheias utilizando o microcontrolador RP2040 (BitDog Lab) com FreeRTOS. O sistema faz leitura periódica de sensores de chuva e nível d'água, processa os dados e aciona atuadores visuais e sonoros (LED RGB, buzzer, display OLED e matriz de LEDs WS2818B) com base em limiares definidos.  

## 🛠️ Funcionalidades Obrigatórias  
✅ Leitura periódica de sensores analógicos via ADC.

✅ Processamento dos dados com decisão de estado (NORMAL ou ALERTA).

✅ Comunicação entre tarefas usando queues do FreeRTOS.

✅ Controle de LED RGB via PWM para indicar o estado do sistema.

✅ Acionamento sonoro com buzzer em caso de alerta.

✅ Feedback visual animado em matriz WS2818B.

✅ Exibição dos valores e status no display OLED via I2C.

✅ Arquitetura modular com múltiplas tarefas FreeRTOS

## 📦 Componentes Utilizados  
- Placa de desenvolvimento: BitDog Lab (RP2040) 
- Sensores: Simulados via potenciômetros conectados aos canais ADC
- LED RGB (PWM nos canais R, G e B)
- Buzzer passivo (controle via frequência)
- Display OLED 128x64 (I2C - SSD1306)
- Matriz de LEDs WS2818B
- Sistema operacional: FreeRTOS 

## ⚙️ Compilação e Gravação  
```bash
git clone https://github.com/Ronaldo8617/Tarefa6-F2.git
cd Tarefa6-F2
mkdir build
cd build
cmake ..
make
```

**Gravação:**  
Pelo ambiente do VScode compile e execute na placa de desnvovimento BitDogLab
Ou
Conecte o RP2040 no modo BOOTSEL e copie o `.uf2` gerado na pasta `build` para a unidade montada.

## 📂 Estrutura do Código  

```plaintext
monitoramento-cheias/  
├── lib/  
│   ├── font.h               # Fonte do display OLED  
│   ├── ssd1306.c, h         # Controle do display SSD1306 via I2C  
│   ├── display_init.c, h    # Inicialização de display  
│   ├── rgb.c, h             # Controle de LED RGB via PWM  
│   ├── buzzer.c, h          # Controle do buzzer passivo  
│   ├── matrixws.c, h        # Controle da matriz WS2818B  
│   ├── ws2812b.pio.h        # Código PIO da WS2818B  
│   ├── FreeRTOSConfig.h      
├── CMakeLists.txt           # Configuração da compilação  
├── Tarefa6-F2.c                   # Código principal com todas as tasks  
├── README.md                # Este documento   
```

## 👨‍💻 Autor  
**Nome:** Ronaldo César Santos Rocha  
**GitHub:** [Ronaldo8617]
