#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

// Define GPIO pins for LEDs
#define GREEN_LED_GPIO GPIO_NUM_38
#define YELLOW_LED_GPIO GPIO_NUM_37
#define RED_LED_GPIO GPIO_NUM_36
#define BLUE_LED_GPIO GPIO_NUM_35

// Define GPIO pins for pushbuttons
#define BUTTON_A_GPIO GPIO_NUM_15
#define BUTTON_B_GPIO GPIO_NUM_14

// Define debounce time in microseconds (50 ms)
#define DEBOUNCE_TIME_US 50000

// Variables to hold counter and step size
static uint8_t counter = 0;
static uint8_t step = 1;

// Variables to track button states and debounce timing
static uint8_t stable_btn_a_state = 0;
static uint8_t stable_btn_b_state = 0;
static uint8_t last_reading_a = 0;
static uint8_t last_reading_b = 0;
static int64_t last_change_time_a = 0;
static int64_t last_change_time_b = 0;

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

// Configure GPIO pins as input
void configure_buttons(void)
{
    gpio_reset_pin(BUTTON_A_GPIO);
    esp_rom_gpio_pad_select_gpio(BUTTON_A_GPIO);
    gpio_set_direction(BUTTON_A_GPIO, GPIO_MODE_INPUT);

    gpio_reset_pin(BUTTON_B_GPIO);
    esp_rom_gpio_pad_select_gpio(BUTTON_B_GPIO);
    gpio_set_direction(BUTTON_B_GPIO, GPIO_MODE_INPUT);
}

// Function to update LED states according to counter value
void update_leds(uint8_t val)
{
    gpio_set_level(GREEN_LED_GPIO,  (val >> 3) & 0x01);
    gpio_set_level(YELLOW_LED_GPIO, (val >> 2) & 0x01);
    gpio_set_level(RED_LED_GPIO,    (val >> 1) & 0x01);
    gpio_set_level(BLUE_LED_GPIO,   (val >> 0) & 0x01);
}

// Function to read and process pushbutton inputs with software debouncing
void process_buttons(void)
{
    int64_t now = esp_timer_get_time();

    // Process Button A (Increments the counter)
    uint8_t reading_a = gpio_get_level(BUTTON_A_GPIO);
    if (reading_a != last_reading_a) {
        last_change_time_a = now;
        last_reading_a = reading_a;
    }

    if ((now - last_change_time_a) > DEBOUNCE_TIME_US) {
        if (reading_a != stable_btn_a_state) {
            stable_btn_a_state = reading_a;
            
            if (stable_btn_a_state == 1) {
                counter = (counter + step) % 16;
                update_leds(counter);
            }
        }
    }

    // Process Button B (Toggles step size between +1 and +2)
    uint8_t reading_b = gpio_get_level(BUTTON_B_GPIO);
    if (reading_b != last_reading_b) {
        last_change_time_b = now;
        last_reading_b = reading_b;
    }

    if ((now - last_change_time_b) > DEBOUNCE_TIME_US) {
        if (reading_b != stable_btn_b_state) {
            stable_btn_b_state = reading_b;

            if (stable_btn_b_state == 1) {
                step = (step == 1) ? 2 : 1;
            }
        }
    }
}

void app_main(void) 
{
    configure_leds();
    configure_buttons();
    update_leds(counter);

    // Read initial states to prevent false trigger at startup
    last_reading_a = gpio_get_level(BUTTON_A_GPIO);
    stable_btn_a_state = last_reading_a;
    
    last_reading_b = gpio_get_level(BUTTON_B_GPIO);
    stable_btn_b_state = last_reading_b;

    int64_t start_time = esp_timer_get_time();
    last_change_time_a = start_time;
    last_change_time_b = start_time;

    while (1) {
        process_buttons();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}