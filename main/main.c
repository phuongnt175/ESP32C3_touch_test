#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/touch_pad.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"

#define TOUCH_PIN1  GPIO_NUM_3
#define TOUCH_PIN2  GPIO_NUM_19
#define TOUCH_PIN3  GPIO_NUM_1
#define TOUCH_PIN4  GPIO_NUM_0

#define LED_1 GPIO_NUM_4
#define LED_2 GPIO_NUM_18 //io18 replace by io2
#define LED_3 GPIO_NUM_6
#define LED_4 GPIO_NUM_7

#define LED_1_BIT 0
#define LED_2_BIT 1
#define LED_3_BIT 2
#define LED_4_BIT 3

#define TOUCH_NUM 4
#define LONG_PRESS_DURATION (100 / portTICK_PERIOD_MS) // Adjust as needed

// Define button states
typedef enum {
    BUTTON_RELEASED,
    BUTTON_PRESSED,
    BUTTON_HOLD,
} ButtonState;

static const int touch_pins[TOUCH_NUM] = {TOUCH_PIN1, TOUCH_PIN2, TOUCH_PIN3, TOUCH_PIN4};
static const int led_pins[TOUCH_NUM] = {LED_1, LED_2, LED_3, LED_4};

// Global status variable to store LED states
uint8_t led_status = 0;  

SemaphoreHandle_t xMutex;

void gpio_init()
{
    esp_rom_gpio_pad_select_gpio(TOUCH_PIN1);
    esp_rom_gpio_pad_select_gpio(TOUCH_PIN2);
    esp_rom_gpio_pad_select_gpio(TOUCH_PIN3);
    esp_rom_gpio_pad_select_gpio(TOUCH_PIN4);

    gpio_set_direction(TOUCH_PIN1, GPIO_MODE_INPUT);
    gpio_set_direction(TOUCH_PIN2, GPIO_MODE_INPUT);
    gpio_set_direction(TOUCH_PIN3, GPIO_MODE_INPUT);
    gpio_set_direction(TOUCH_PIN4, GPIO_MODE_INPUT);

}

void led_init()
{
    esp_rom_gpio_pad_select_gpio(LED_1);
    esp_rom_gpio_pad_select_gpio(LED_2);
    esp_rom_gpio_pad_select_gpio(LED_3);
    esp_rom_gpio_pad_select_gpio(LED_4);

    gpio_set_direction(LED_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_2, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_3, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_4, GPIO_MODE_OUTPUT);
}

// Function to set the state of an LED in the global status variable
void set_led_state(int led, bool state)
{
    int bit_position;

    switch (led)
    {
    case LED_1:
        bit_position = LED_1_BIT;
        break;
    case LED_2:
        bit_position = LED_2_BIT;
        break;
    case LED_3:
        bit_position = LED_3_BIT;
        break;
    case LED_4:
        bit_position = LED_4_BIT;
        break;
    default:
        return;  // Invalid LED
    }

    if (state)
    {
        led_status |= (1 << bit_position);  // Set the bit to 1
    }
    else
    {
        led_status &= ~(1 << bit_position);  // Set the bit to 0
    }

    // Now you can use gpio_set_level to update the physical LED state if needed
    gpio_set_level(led, state);
}

// Function to get the state of an LED from the global status variable
bool get_led_state(int led)
{
    int bit_position;

    switch (led)
    {
    case LED_1:
        bit_position = LED_1_BIT;
        break;
    case LED_2:
        bit_position = LED_2_BIT;
        break;
    case LED_3:
        bit_position = LED_3_BIT;
        break;
    case LED_4:
        bit_position = LED_4_BIT;
        break;
    default:
        return false;  // Invalid LED
    }

    return (led_status & (1 << bit_position)) != 0;
}

static void toggle_led(int led_pin)
{
// Get the current state of the LED
    bool current_state = get_led_state(led_pin);

    // Toggle the state
    set_led_state(led_pin, !current_state);
}

static void button_task(void *pvParameter)
{
    TickType_t press_start_time[TOUCH_NUM] = {0};
    bool led_toggled[TOUCH_NUM] = {false};

    while (1)
    {
        if (xSemaphoreTake(xMutex, portMAX_DELAY))
        {
            for (int i = 0; i < TOUCH_NUM; i++)
            {
                //ESP_LOGI("MAIN", "touch %d get status: %d", i, gpio_get_level(touch_pins[i]));
                if (gpio_get_level(touch_pins[i]) == 0)
                {
                    // Button is pressed
                    if (!led_toggled[i])
                    {
                        // Button was just pressed, and the LED state has not been toggled
                        printf("%d touch (press)\n", i + 1);
                        toggle_led(led_pins[i]);  // Toggle the LED state only once
                        led_toggled[i] = true;
                    }

                    TickType_t press_duration = xTaskGetTickCount() - press_start_time[i];
                    if (press_duration >= LONG_PRESS_DURATION && !led_toggled[i])
                    {
                        printf("%d touch (hold)\n", i + 1);
                        // Perform actions for holding the button
                        // Add your code here if needed
                    }
                }
                else
                {
                    // Button is released
                    if (press_start_time[i] != 0)
                    {
                        // Button was released
                        printf("%d touch (release)\n", i + 1);
                        press_start_time[i] = 0;
                        led_toggled[i] = false;  // Reset the LED state for the next press
                    }
                }
                // Update the press start time if the button is pressed
                if (gpio_get_level(touch_pins[i]) == 0 && press_start_time[i] == 0)
                {
                    press_start_time[i] = xTaskGetTickCount();
                }
            }

            xSemaphoreGive(xMutex);
        }

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}



void app_main(void)
{
    gpio_init();
    led_init();
    xMutex = xSemaphoreCreateMutex();

    vTaskDelay( 1000 / portTICK_PERIOD_MS);
    //xTaskCreate(&tp_example_read_task, "touch_pad_read_task", 4096, NULL, 5, NULL);
    xTaskCreate(&button_task, "button_task", 4096, NULL, 5, NULL);
} 