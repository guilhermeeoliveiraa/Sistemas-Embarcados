#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "driver/ledc.h"

// Define GPIO pins for counter LEDs
#define GREEN_LED_GPIO GPIO_NUM_38
#define YELLOW_LED_GPIO GPIO_NUM_37
#define RED_LED_GPIO GPIO_NUM_36
#define BLUE_LED_GPIO GPIO_NUM_35

// Define GPIO pins for White LED and Buzzer
#define BUZZER_GPIO GPIO_NUM_1
#define WHITE_LED_GPIO GPIO_NUM_2

// Define GPIO pins for pushbuttons
#define BUTTON_A_GPIO GPIO_NUM_15
#define BUTTON_B_GPIO GPIO_NUM_14

// Define debounce time in microseconds (30ms)
#define DEBOUNCE_TIME_US 30000

// Counter and step variables
static uint8_t counter = 0;
static uint8_t step = 1;

// Timer handles for debounce
static esp_timer_handle_t btn_a_timer_handle = NULL;
static esp_timer_handle_t btn_b_timer_handle = NULL;

// Forward declarations for timer callbacks
static void btn_a_timer_callback(void* arg);
static void btn_b_timer_callback(void* arg);

// Configure LED GPIOs as outputs
void configure_leds(void)
{
  gpio_reset_pin(GREEN_LED_GPIO);
  esp_rom_gpio_pad_select_gpio(GREEN_LED_GPIO);
  gpio_set_direction(GREEN_LED_GPIO, GPIO_MODE_OUTPUT);

  gpio_reset_pin(YELLOW_LED_GPIO);
  esp_rom_gpio_pad_select_gpio(YELLOW_LED_GPIO);
  gpio_set_direction(YELLOW_LED_GPIO, GPIO_MODE_OUTPUT);

  gpio_reset_pin(RED_LED_GPIO);
  esp_rom_gpio_pad_select_gpio(RED_LED_GPIO);
  gpio_set_direction(RED_LED_GPIO, GPIO_MODE_OUTPUT);

  gpio_reset_pin(BLUE_LED_GPIO);
  esp_rom_gpio_pad_select_gpio(BLUE_LED_GPIO);
  gpio_set_direction(BLUE_LED_GPIO, GPIO_MODE_OUTPUT);
}

// Configure PWM channels for buzzer and white LED
void configure_pwm(void)
{
  ledc_timer_config_t ledc_timer = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .timer_num = LEDC_TIMER_0,
    .duty_resolution = LEDC_TIMER_12_BIT,
    .freq_hz = 1000,
    .clk_cfg = LEDC_AUTO_CLK
  };
  ledc_timer_config(&ledc_timer);

  ledc_timer_config_t buzzer_timer = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .timer_num = LEDC_TIMER_1,
    .duty_resolution = LEDC_TIMER_12_BIT,
    .freq_hz = 1000,
    .clk_cfg = LEDC_AUTO_CLK
  };
  ledc_timer_config(&buzzer_timer);

  ledc_channel_config_t channel_0 = {
    .gpio_num = BUZZER_GPIO,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER_0,
    .duty = 0,
    .hpoint = 0
  };
  ledc_channel_config(&channel_0);

  ledc_channel_config_t channel_1 = {
    .gpio_num = WHITE_LED_GPIO,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_1,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER_1,
    .duty = 0,
    .hpoint = 0
  };
  ledc_channel_config(&channel_1);
}

// Update LEDs, buzzer, and white LED based on counter value
void update_leds(uint8_t val)
{
  gpio_set_level(GREEN_LED_GPIO,  (val >> 3) & 0x01);
  gpio_set_level(YELLOW_LED_GPIO, (val >> 2) & 0x01);
  gpio_set_level(RED_LED_GPIO,    (val >> 1) & 0x01);
  gpio_set_level(BLUE_LED_GPIO,   (val >> 0) & 0x01);

  // Update Buzzer
  uint32_t buzzer_duty = (val * 4095) / 15;
  uint32_t frequency = 500 + (val * 250);

  ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, frequency);
  ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, buzzer_duty);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

  // Update White LED
  uint32_t white_duty = (val * 4095) / 15;

  ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, white_duty);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

// GPIO ISR handler: disables interrupt and starts debounce timer
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
  uint32_t gpio_num = (uint32_t) arg;

  if (gpio_num == BUTTON_A_GPIO) {
    gpio_intr_disable(BUTTON_A_GPIO);
    esp_timer_start_once(btn_a_timer_handle, DEBOUNCE_TIME_US);
  } else if (gpio_num == BUTTON_B_GPIO) {
    gpio_intr_disable(BUTTON_B_GPIO);
    esp_timer_start_once(btn_b_timer_handle, DEBOUNCE_TIME_US);
  }
}

// Button A timer callback: checks state, updates counter, re-enables interrupt
static void btn_a_timer_callback(void* arg)
{
  if (gpio_get_level(BUTTON_A_GPIO) == 1) {
    counter = (counter + step) % 16;
    update_leds(counter);
  }
  gpio_intr_enable(BUTTON_A_GPIO);
}

// Button B timer callback: checks state, updates counter, re-enables interrupt
static void btn_b_timer_callback(void* arg)
{
  if (gpio_get_level(BUTTON_B_GPIO) == 1) {
    if (counter >= step)
      counter -= step;
    else
      counter = 15;
    update_leds(counter);
  }
  gpio_intr_enable(BUTTON_B_GPIO);
}

// Configure buttons as inputs and create esp_timers
void configure_buttons(void)
{
  gpio_config_t io_conf = {
    .intr_type = GPIO_INTR_POSEDGE,
    .mode = GPIO_MODE_INPUT,
    .pin_bit_mask = (1ULL << BUTTON_A_GPIO) | (1ULL << BUTTON_B_GPIO),
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .pull_up_en = GPIO_PULLDOWN_DISABLE
  };
  gpio_config(&io_conf);

  const esp_timer_create_args_t btn_a_timer_args = {
    .callback = &btn_a_timer_callback,
    .name = "btn_a_debounce"
  };
  esp_timer_create(&btn_a_timer_args, &btn_a_timer_handle);

  const esp_timer_create_args_t btn_b_timer_args = {
    .callback = &btn_b_timer_callback,
    .name = "btn_b_debounce"
  };
  esp_timer_create(&btn_b_timer_args, &btn_b_timer_handle);
}

// Install ISR service and add handlers
void configure_isr(void)
{
  gpio_install_isr_service(0);
  gpio_isr_handler_add(BUTTON_A_GPIO, gpio_isr_handler, (void*) BUTTON_A_GPIO);
  gpio_isr_handler_add(BUTTON_B_GPIO, gpio_isr_handler, (void*) BUTTON_B_GPIO);
}

void app_main(void) 
{
  configure_leds();
  configure_buttons();
  configure_isr();
  configure_pwm();
  update_leds(counter);

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}