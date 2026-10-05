#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define LED_GPIO GPIO_NUM_1
#define BUTTON_GPIO GPIO_NUM_10

#define DEBOUNCE_TIME 50000
#define LONG_PRESS_TIME 2000000
#define ACTIVE_TIMEOUT 10000000

static esp_timer_handle_t debounce_timer_handle = NULL;
static esp_timer_handle_t long_press_timer_handle = NULL;
static esp_timer_handle_t timeout_timer_handle = NULL;

static void debounce_timer_callback(void* arg);
static void long_press_timer_callback(void* arg);
static void timeout_timer_callback(void* arg);

void configure_led(void)
{
  gpio_reset_pin(LED_GPIO);
  gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
  gpio_set_level(LED_GPIO, 0);
}

void configure_button(void)
{
  gpio_config_t io_config = {
    .intr_type = GPIO_INTR_ANYEDGE,
    .mode = GPIO_MODE_INPUT,
    .pin_bit_mask = (1ULL << BUTTON_GPIO),
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .pull_up_en = GPIO_PULLUP_ENABLE
  };
  gpio_config(&io_config);
}

void create_timers(void)
{
  const esp_timer_create_args_t debounce_args = {
    .callback = &debounce_timer_callback,
    .name = "debounce_timer"    
  };
  esp_timer_create(&debounce_args, &debounce_timer_handle);

  const esp_timer_create_args_t long_press_args = {
    .callback = &long_press_timer_callback,
    .name = "long_press_timer"    
  };
  esp_timer_create(&long_press_args, &long_press_timer_handle);

  const esp_timer_create_args_t timeout_args = {
    .callback = &timeout_timer_callback,
    .name = "timeout_timer"    
  };
  esp_timer_create(&timeout_args, &timeout_timer_handle);
}

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
  gpio_intr_disable(BUTTON_GPIO);
  esp_timer_start_once(debounce_timer_handle, DEBOUNCE_TIME);
}

static void debounce_timer_callback(void* arg)
{
  int btn_level = gpio_get_level(BUTTON_GPIO);

  if (btn_level == 0) {
    esp_timer_start_once(long_press_timer_handle, LONG_PRESS_TIME);
  } else {
    if (esp_timer_is_active(long_press_timer_handle)) {
      esp_timer_stop(long_press_timer_handle);
      gpio_set_level(LED_GPIO, 1);
      esp_timer_stop(timeout_timer_handle);
      esp_timer_start_once(timeout_timer_handle, ACTIVE_TIMEOUT);
    }
  }

  gpio_intr_enable(BUTTON_GPIO);
}

static void long_press_timer_callback(void* arg)
{
  if (gpio_get_level(BUTTON_GPIO) == 0) {
    gpio_set_level(LED_GPIO, 0);
    esp_timer_stop(timeout_timer_handle);
  }
}

static void timeout_timer_callback(void* arg)
{
  gpio_set_level(LED_GPIO, 0);
}

void configure_isr(void)
{
  gpio_install_isr_service(0);
  gpio_isr_handler_add(BUTTON_GPIO, gpio_isr_handler, (void*) BUTTON_GPIO);
}

void app_main(void) 
{
  configure_led();
  configure_button();
  create_timers();
  configure_isr();

  while (true)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}