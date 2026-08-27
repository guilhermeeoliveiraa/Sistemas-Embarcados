#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

// Define GPIO pins for LEDs
#define GREEN_LED_GPIO GPIO_NUM_38
#define YELLOW_LED_GPIO GPIO_NUM_37
#define RED_LED_GPIO GPIO_NUM_36
#define BLUE_LED_GPIO GPIO_NUM_35

// Define delay time in milliseconds
#define DELAY_TIME_MS 500


// Variable to hold the state of LEDs
uint8_t leds_state = 0;

// Configure GPIO pins as output
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

// Function to control LED states in phase 1
void fase1(void)
{
    for (int i = 0; i < 16; i++)
    {
        gpio_set_level(GREEN_LED_GPIO,  (leds_state >> 3) & 1);
        gpio_set_level(YELLOW_LED_GPIO, (leds_state >> 2) & 1);
        gpio_set_level(RED_LED_GPIO,    (leds_state >> 1) & 1);
        gpio_set_level(BLUE_LED_GPIO,   (leds_state >> 0) & 1);
        leds_state = (leds_state + 1) % 16; // Cycle through 0-15
        vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    }
}

// Function to control LED states in phase 2
void fase2(void)
{
    // Turn off all LEDs
    gpio_set_level(GREEN_LED_GPIO, 0);
    gpio_set_level(YELLOW_LED_GPIO, 0);
    gpio_set_level(RED_LED_GPIO, 0);
    gpio_set_level(BLUE_LED_GPIO, 0);


    // toggle each LED one by one of LSB to MSB
    gpio_set_level(GREEN_LED_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    gpio_set_level(GREEN_LED_GPIO, 0);
    gpio_set_level(YELLOW_LED_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    gpio_set_level(YELLOW_LED_GPIO, 0);
    gpio_set_level(RED_LED_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    gpio_set_level(RED_LED_GPIO, 0);
    gpio_set_level(BLUE_LED_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    gpio_set_level(BLUE_LED_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));


    // toggle all LEDs together of MSB to LSB
    gpio_set_level(BLUE_LED_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    gpio_set_level(BLUE_LED_GPIO, 0);
    gpio_set_level(RED_LED_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    gpio_set_level(RED_LED_GPIO, 0);
    gpio_set_level(YELLOW_LED_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    gpio_set_level(YELLOW_LED_GPIO, 0);
    gpio_set_level(GREEN_LED_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    gpio_set_level(GREEN_LED_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
}

void app_main(void)
{
    configure_leds();
    while (1)
    {
        fase1();
        fase2();
    }
}
