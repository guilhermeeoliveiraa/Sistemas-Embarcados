#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "driver/ledc.h"

// Define GPIO pins for LEDs
#define GREEN_LED_GPIO GPIO_NUM_38
#define YELLOW_LED_GPIO GPIO_NUM_37
#define RED_LED_GPIO GPIO_NUM_36
#define BLUE_LED_GPIO GPIO_NUM_35

#define BUZZER_GPIO GPIO_NUM_1
#define WHITE_LED_GPIO GPIO_NUM_2

// Define GPIO pins for pushbuttons
#define BUTTON_A_GPIO GPIO_NUM_15
#define BUTTON_B_GPIO GPIO_NUM_14

// Define debounce time in microseconds
#define DEBOUNCE_TIME_US 30000

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

//Configure PWM Channel
void configure_pwm(void)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_timer_config_t buzzer_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_1,
        .duty_resolution = LEDC_TIMER_13_BIT,
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

    // Update Buzzer
    uint32_t duty = 4095;
    uint32_t frequency = 500 + (val * 250);

    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, frequency);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    // Update White LED
    uint32_t white_duty = (val * 8191) / 15;

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, white_duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
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

    // Process Button B (Decrement the counter)
    uint8_t reading_b = gpio_get_level(BUTTON_B_GPIO);
    if (reading_b != last_reading_b) {
        last_change_time_b = now;
        last_reading_b = reading_b;
    }

    if ((now - last_change_time_b) > DEBOUNCE_TIME_US) {
        if (reading_b != stable_btn_b_state) {
            stable_btn_b_state = reading_b;

            if (stable_btn_b_state == 1) {
                if (counter >= step)
                    counter -= step;
                else
                    counter = 15;
                update_leds(counter);
            }
        }
    }
}

void app_main(void) 
{
    configure_leds();
    configure_buttons();
    configure_pwm();
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