#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_GPIO GPIO_NUM_1
#define TX_GPIO GPIO_NUM_17
#define RX_GPIO GPIO_NUM_16

#define BUF_SIZE 32

void configure_led(void)
{
  gpio_reset_pin(LED_GPIO);
  gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
  gpio_set_level(LED_GPIO, 0);
}

void configure_uart(void)
{
  uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
  };

  uart_param_config(UART_NUM_2, &uart_config);

  uart_driver_install(
    UART_NUM_2,         // Porta UART que está sendo ativada
    2048,               // Tamanho do buffer de recepção (RX) em bytes
    2048,               // Tamanho do buffer de transmissão (TX) em bytes (0 se não quiser usar buffer para envio)
    0,                  // Tamanho da fila de eventos (quantos eventos ela pode guardar por vez)
    NULL,               // Ponteiro para a fila de eventos (pode ser NULL se você não for usar eventos)
    0                   // Flags para alocação da interrupção (ex: ESP_INTR_FLAG_IRAM)
  );

  uart_set_pin(
    UART_NUM_2,               // O número da UART (UART_NUM_0, UART_NUM_1 ou UART_NUM_2)
    TX_GPIO,                  // O número do pino GPIO que vai Transmitir dados (TX)
    RX_GPIO,                  // O número do pino GPIO que vai Receber dados (RX)
    UART_PIN_NO_CHANGE,       // O número do pino para controle de fluxo RTS (ou UART_PIN_NO_CHANGE)
    UART_PIN_NO_CHANGE        // O número do pino para controle de fluxo CTS (ou UART_PIN_NO_CHANGE)
  );
}

// Função que será executada como uma Task em segundo plano
void uart_rx_task(void *pvParameters)
{
  // Cria um buffer (vetor) na memória para guardar os bytes que vamos ler
  uint8_t dados_recebidos[BUF_SIZE];

  while (1) {
    // Lê os dados do buffer da UART
    int tamanho_dados = uart_read_bytes(UART_NUM_2, dados_recebidos, BUF_SIZE - 1, pdMS_TO_TICKS(100));
    
    // Se retornou um número maior que zero, significa que chegaram bytes!
    if (tamanho_dados > 0) {
      // Adiciona o caractere nulo '\0' no final para tratar os dados como uma string em C
      dados_recebidos[tamanho_dados] = '\0';
      
      // Imprime no console principal (UART0) o que foi recebido na UART2
      printf("LIDO DA UART2: '%s' [%d bytes]\n", (char*)dados_recebidos, tamanho_dados);

      // Tomada de decisão com o texto recebido
      if (strcmp((char*)dados_recebidos, "LIGAR") == 0) {
        gpio_set_level(LED_GPIO, 1);
      }
      if (strcmp((char*)dados_recebidos, "DESLIGAR") == 0) {
        gpio_set_level(LED_GPIO, 0);
      }
    }
  }
}

void app_main(void) 
{
  configure_led();
  configure_uart();

  const char* on = "LIGAR";
  const char* off = "DESLIGAR";

  xTaskCreate(uart_rx_task, "uart_rx_task", 4096, NULL, 10, NULL);

  while (true)
  {
    printf("ENVIANDO PELA UART2: '%s'\n", on);
    uart_write_bytes(UART_NUM_2, on, strlen(on));
    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("ENVIANDO PELA UART2: '%s'\n", off);
    uart_write_bytes(UART_NUM_2, off, strlen(off));
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}