#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

// Definição dos Pinos
#define LED_GPIO          GPIO_NUM_4
#define BUTTON_GPIO       GPIO_NUM_10

// Definição dos Tempos (em microsegundos - us)
#define DEBOUNCE_DELAY_US  50000LL     // 50 ms para debounce
#define TIMEOUT_30S_US    30000000LL   // 30 segundos
#define LONG_PRESS_US      2000000LL   // 2 segundos para clique longo

void app_main(void) {
  // Configuração do pino do LED como saída
  gpio_reset_pin(LED_GPIO);
  gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
  gpio_set_level(LED_GPIO, 0);

  // Configuração do pino do Botão como entrada (Floating devido ao Pull-Up externo de 10k)
  gpio_reset_pin(BUTTON_GPIO);
  gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
  gpio_set_pull_mode(BUTTON_GPIO, GPIO_FLOATING);

  // Variáveis do Debounce e Leitura do Botão
  int last_raw_state = 1;
  int debounced_state = 1;
  int64_t last_debounce_time = 0;

  // Variáveis para Controlo do Pressionamento Longo
  int64_t press_start_time = 0;
  bool long_press_handled = false;

  // Variáveis de Estado do LED e Temporizador de 30s
  bool led_state = false;
  int64_t led_turn_on_time = 0;

  printf("Sistema de Iluminacao Iniciado (Modo Polling Nao-Bloqueante)\n");

  while (1) {
    int64_t now = esp_timer_get_time();
    int current_raw_state = gpio_get_level(BUTTON_GPIO);

    // 1. TRATAMENTO DE DEBOUNCE POR SOFTWARE (NÃO-BLOQUEANTE)
    if (current_raw_state != last_raw_state) {
      last_debounce_time = now;
      last_raw_state = current_raw_state;
    }

    if ((now - last_debounce_time) > DEBOUNCE_DELAY_US) {
      // Se o sinal estabilizou e houve alteração do estado aceite
      if (current_raw_state != debounced_state) {
        debounced_state = current_raw_state;

        // Transição: Botão Pressionado (1 -> 0)
        if (debounced_state == 0) {
          press_start_time = now;
          long_press_handled = false;
        }
        // Transição: Botão Solto (0 -> 1)
        else {
          // Processa clique curto apenas se não tiver sido tratado como longo (>2s)
          if (!long_press_handled) {
            if (!led_state) {
              // Condição 1: LED estava apagado -> Acende e inicia contagem de 30s
              led_state = true;
              gpio_set_level(LED_GPIO, 1);
              led_turn_on_time = now;
              printf("LED Aceso! Temporizador de 30s iniciado.\n");
            } else {
              // Condição 2: LED estava aceso -> Reinicia temporizador para 30s
              led_turn_on_time = now;
              printf("Temporizador de 30s reiniciado!\n");
            }
          }
        }
      }
    }

    // 2. CONDIÇÃO 3: DESLIGAMENTO MANUAL POR PRESSIONAMENTO LONGO (> 2s)
    if (debounced_state == 0 && !long_press_handled) {
      if ((now - press_start_time) >= LONG_PRESS_US) {
        led_state = false;
        gpio_set_level(LED_GPIO, 0);
        long_press_handled = true; // Impede que o disparo de soltar o botão volte a ligar o LED
        printf("Desligamento manual efetuado (Pressionamento > 2s).\n");
      }
    }

    // 3. TEMPORIZADOR DE 30 SEGUNDOS (DESLIGAMENTO AUTOMÁTICO)
    if (led_state) {
      if ((now - led_turn_on_time) >= TIMEOUT_30S_US) {
        led_state = false;
        gpio_set_level(LED_GPIO, 0);
        printf("Tempo de 30s esgotado. LED Apagado automaticamente.\n");
      }
    }
  }
}