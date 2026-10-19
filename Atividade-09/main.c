#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

// Definições de Pinos e Parâmetros
#define LED_GPIO            GPIO_NUM_1
#define BUTTON_GPIO         GPIO_NUM_10
#define ADC_CHANNEL         ADC_CHANNEL_4    // GPIO 5 no ESP32-S3
#define ADC_ATTEN           ADC_ATTEN_DB_12  // Escala de 0 a 3.3V
#define ADC_WIDTH           ADC_BITWIDTH_12  // Resolução de 12 bits (0 - 4095)

#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO      LED_GPIO
#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_DUTY_RES       LEDC_TIMER_10_BIT // Resolução de 10 bits (0 - 1023)
#define LEDC_FREQUENCY      5000              // 5 kHz

// Leitura máxima esperada devido ao divisor de tensão 10k/10k (1.65V max)
#define ADC_MAX_EXPECTED    2047

// Estados do Sistema
typedef enum {
    MODE_LIVE = 0,
    MODE_HOLD
} system_mode_t;

// Variáveis Globais de Controle
static volatile system_mode_t current_mode = MODE_LIVE;
static volatile uint32_t last_button_press_time = 0;
static adc_oneshot_unit_handle_t adc1_handle;

// ISR / Tratamento do Botão com Debounce
static void IRAM_ATTR button_isr_handler(void* arg) {
    int64_t current_time_ms = esp_timer_get_time() / 1000;
    
    if ((current_time_ms - last_button_press_time) > 200) {
        last_button_press_time = current_time_ms;
        current_mode = (current_mode == MODE_LIVE) ? MODE_HOLD : MODE_LIVE;
    }
}

// Configuração do Periférico PWM (LEDC)
static void init_ledc(void) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

// Configuração do Conversor ADC
static void init_adc(void) {
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_WIDTH,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config));
}

// Configuração do Botão com Pull-Up
static void init_button(void) {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .pull_up_en = GPIO_PULLUP_ENABLE,    // Habilitado Pull-up interno
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, (void*) BUTTON_GPIO);
}

void app_main(void) {
    init_ledc();
    init_adc();
    init_button();

    int adc_raw = 0;
    uint32_t voltage_mv = 0;
    uint32_t pwm_duty = 0;
    
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t telemetry_interval = pdMS_TO_TICKS(500);

    while (1) {
        if (current_mode == MODE_LIVE) {
            ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL, &adc_raw));
            
            // 1. Tensão REAL lida no pino (escala real do ADC de 0 a 4095 -> 0 a 3300mV)
            voltage_mv = ((uint32_t)adc_raw * 3300) / 4095;

            // 2. Trava de segurança para tolerância dos resistores (máximo 1.65V = 2047)
            uint32_t adc_clamped = adc_raw;
            if (adc_clamped > 2047) {
                adc_clamped = 2047;
            }

            // 3. Mapeamento para o PWM: 0..2047 (0..1.65V) vira 0..1023 (0..100% Brilho)
            pwm_duty = (adc_clamped * 1023) / 2047;
            
            ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, pwm_duty);
            ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
        }

        // Telemetria via UART sem bloquear a execução
        if ((xTaskGetTickCount() - last_wake_time) >= telemetry_interval) {
            last_wake_time = xTaskGetTickCount();
            
            printf("ADC Raw: %4d | Tensão: %4lu mV | Estado: %s | PWM Duty: %lu\n",
                   adc_raw,
                   voltage_mv,
                   (current_mode == MODE_LIVE) ? "LIVE" : "HOLD",
                   pwm_duty);
        }

        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}