#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "lib/ssd1306.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C
#define WIDTH 128
#define HEIGHT 64

#define BOTAO_ENTRADA 5     // Botão A
#define BOTAO_SAIDA 6       // Botão B
#define BOTAO_RESET 22

#define LED_R 13
#define LED_G 11
#define LED_B 12
#define BUZZER_PIN 21
#define MAX_USUARIOS 8

ssd1306_t ssd;

SemaphoreHandle_t xSemUsuarios;
SemaphoreHandle_t xSemReset;
SemaphoreHandle_t xMutexDisplay;

uint8_t usuarios_ativos = 0;

// === Funções Auxiliares ===

void beep_buzzer(int duration_ms, int freq) {
    uint slice_buzzer = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint channel_buzzer = pwm_gpio_to_channel(BUZZER_PIN);
    
    // Configuração do PWM para o buzzer
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.0f); // Divisor de clock 1 (125MHz)
    
    // Calcula o wrap value baseado na frequência desejada
    uint32_t wrap = (125000000 / freq) - 1;
    if (wrap > 65535) wrap = 65535; // Limite máximo para 16-bit
    
    pwm_config_set_wrap(&config, wrap);
    pwm_init(slice_buzzer, &config, true);
    
    // Configura duty cycle para 50%
    pwm_set_chan_level(slice_buzzer, channel_buzzer, wrap / 2);
    
    // Aguarda a duração do beep
    sleep_ms(duration_ms);
    
    // Desliga o buzzer
    pwm_set_chan_level(slice_buzzer, channel_buzzer, 0);
    pwm_set_enabled(slice_buzzer, false);
}

void atualizar_led(uint8_t qtd) {
    if (qtd == 0) {
        // Nenhum usuário — azul
        gpio_put(LED_R, 0);
        gpio_put(LED_G, 0);
        gpio_put(LED_B, 1);
    } else if (qtd == MAX_USUARIOS) {
        // Capacidade máxima — vermelho
        gpio_put(LED_R, 1);
        gpio_put(LED_G, 0);
        gpio_put(LED_B, 0);
    } else if (qtd == MAX_USUARIOS - 1) {
        // Apenas 1 vaga restante — amarelo (vermelho + verde)
        gpio_put(LED_R, 1);
        gpio_put(LED_G, 1);
        gpio_put(LED_B, 0);
    } else {
        // Usuários ativos (entre 1 e MAX-2) — verde
        gpio_put(LED_R, 0);
        gpio_put(LED_G, 1);
        gpio_put(LED_B, 0);
    }
}

void atualizar_display(const char* msg, uint8_t qtd) {
    if (xSemaphoreTake(xMutexDisplay, portMAX_DELAY) == pdTRUE) {
        char buffer[32];
        ssd1306_fill(&ssd, 0);
        ssd1306_draw_string(&ssd, msg, 0, 10);
        sprintf(buffer, "Usuarios: %d", qtd);
        ssd1306_draw_string(&ssd, buffer, 0, 30);
        ssd1306_send_data(&ssd);
        xSemaphoreGive(xMutexDisplay);
    }
}

// === Tarefas ===

void vTaskEntrada(void *params) {
    while (1) {
        if (gpio_get(BOTAO_ENTRADA) == 0) {
            if (uxSemaphoreGetCount(xSemUsuarios) < MAX_USUARIOS) {
                xSemaphoreGive(xSemUsuarios);
                usuarios_ativos++;
                atualizar_display("Entrada        autorizada", usuarios_ativos);
            } else {
                atualizar_display("Capacidade     cheia!", usuarios_ativos);
                beep_buzzer(200, 1000);;
            }
            atualizar_led(usuarios_ativos);
            vTaskDelay(pdMS_TO_TICKS(300));
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void vTaskSaida(void *params) {
    while (1) {
        if (gpio_get(BOTAO_SAIDA) == 0) {
            if (xSemaphoreTake(xSemUsuarios, 0) == pdTRUE && usuarios_ativos > 0) {
                usuarios_ativos--;
                atualizar_display("Saida          autorizada", usuarios_ativos);
                atualizar_led(usuarios_ativos);
            }
            vTaskDelay(pdMS_TO_TICKS(300));
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void vTaskReset(void *params) {
    while (1) {
        if (xSemaphoreTake(xSemReset, portMAX_DELAY)) {
            for (int i = 0; i < usuarios_ativos; i++) {
                xSemaphoreTake(xSemUsuarios, 0);
            }
            usuarios_ativos = 0;
            atualizar_display("Resetar sistema", usuarios_ativos);
            atualizar_led(usuarios_ativos);
            beep_buzzer(100, 1000);
            beep_buzzer(100, 1000);
        }
    }
}

// === Interrupção do botão RESET ===

void gpio_reset_irq(uint gpio, uint32_t events) {
    if (gpio == BOTAO_RESET) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xSemReset, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

// === Main ===

int main() {
    stdio_init_all();

    // Inicializa I2C e display
    i2c_init(I2C_PORT, 400000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    // GPIOs
    gpio_init(BOTAO_ENTRADA);
    gpio_set_dir(BOTAO_ENTRADA, GPIO_IN);
    gpio_pull_up(BOTAO_ENTRADA);

    gpio_init(BOTAO_SAIDA);
    gpio_set_dir(BOTAO_SAIDA, GPIO_IN);
    gpio_pull_up(BOTAO_SAIDA);

    gpio_init(BOTAO_RESET);
    gpio_set_dir(BOTAO_RESET, GPIO_IN);
    gpio_pull_up(BOTAO_RESET);

    gpio_set_irq_enabled_with_callback(BOTAO_RESET, GPIO_IRQ_EDGE_FALL, true, &gpio_reset_irq);

    gpio_init(LED_R);
    gpio_init(LED_G);
    gpio_init(LED_B);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_set_dir(LED_B, GPIO_OUT);
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    // Semáforos e mutex
    xSemUsuarios = xSemaphoreCreateCounting(MAX_USUARIOS, 0);
    xSemReset = xSemaphoreCreateBinary();
    xMutexDisplay = xSemaphoreCreateMutex();

    // Estado inicial
    atualizar_display("Sistema Pronto", usuarios_ativos);
    atualizar_led(usuarios_ativos);

    // Cria tarefas
    xTaskCreate(vTaskEntrada, "Entrada", 256, NULL, 1, NULL);
    xTaskCreate(vTaskSaida, "Saida", 256, NULL, 1, NULL);
    xTaskCreate(vTaskReset, "Reset", 256, NULL, 2, NULL);

    // Inicia scheduler
    vTaskStartScheduler();
    while (true); // Nunca deve chegar aqui
}

