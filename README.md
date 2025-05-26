# 🔐 **Sistema de Controle de Acesso com FreeRTOS e Pi Pico**

Projeto da área de sistemas embarcados para a **Residência TIC 37 - Embarcatech**, que simula o controle de entrada e saída de usuários em um ambiente com capacidade limitada. Utiliza botões físicos, LEDs RGB, buzzer e display OLED SSD1306, integrando multitarefas com o sistema operacional **FreeRTOS** na plataforma **BitDogLab**.

---

## 🎯 **Objetivos**

Controlar o acesso de até 8 usuários, com sinalizações visuais e sonoras, usando semáforos e interrupções em FreeRTOS. O sistema fornece feedback em tempo real por meio de display OLED e LEDs coloridos conforme o nível de ocupação.

---

## 🎥 **Demonstração**

[Ver Vídeo do Projeto](https://drive.google.com/file/d/1-v1h6rC1pmx5e1LECZpEIkMeguFnmjnc/view?usp=drive_link)  


---

## 🛠️ **Tecnologias Utilizadas**

- **Linguagem de Programação:** C / CMake  
- **Placa Microcontroladora:** BitDogLab (Raspberry Pi Pico)  
- **Sistema Operacional:** FreeRTOS  
- **Periféricos:**  
  - Display OLED SSD1306 (I2C)  
  - LEDs RGB (GPIO)  
  - Buzzer (PWM)  
  - Botões físicos (GPIO com pull-up e interrupções)

---

## 📖 **Como Funciona**

- **Botão A (GPIO 5):** Entrada de usuário  
- **Botão B (GPIO 6):** Saída de usuário  
- **Botão Reset (GPIO 22):** Zera o sistema (via interrupção)  

O sistema permite até 8 usuários ativos. Cada ação é gerenciada por tarefas separadas em FreeRTOS, usando semáforos para controle de acesso.

### Comportamento dos LEDs RGB:
- 🔵 **Azul:** Nenhum usuário  
- 🟢 **Verde:** Ocupação parcial  
- 🟡 **Amarelo:** Última vaga  
- 🔴 **Vermelho:** Capacidade cheia

### Alertas:
- Display OLED exibe mensagens como “Entrada autorizada” ou “Capacidade cheia!”  
- Buzzer emite som em caso de erro ou reset do sistema

---

## ⚙️ **Funcionalidades Demonstradas**

- Controle de acesso com semáforo de contagem (`xSemaphoreCreateCounting`)  
- Reset do sistema com semáforo binário acionado por interrupção (`xSemaphoreGiveFromISR`)  
- Exclusão mútua no acesso ao display OLED com `xSemaphoreCreateMutex`  
- Sinalização com LEDs RGB usando GPIO  
- Geração de alerta sonoro com PWM no buzzer  
- Organização multitarefa com FreeRTOS (`xTaskCreate`, `vTaskDelay`)  
- Interrupção externa com `gpio_set_irq_enabled_with_callback`
