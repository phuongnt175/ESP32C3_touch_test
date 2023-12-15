#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/touch_pad.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"

#define TOUCH_PIN1  3
#define TOUCH_PIN2  19
#define TOUCH_PIN3  1
#define TOUCH_PIN4  0

#define LED_1 4
#define LED_2 18
#define LED_3 6
#define LED_4 7

#define LED_1_BIT 0
#define LED_2_BIT 1
#define LED_3_BIT 2
#define LED_4_BIT 3

#define TOUCH_NUM 4
#define LONG_PRESS_DURATION 500 / portTICK_PERIOD_MS // Adjust as needed

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

/*
  Read values sensed at all available touch pads.
 Print out values in a loop on a serial monitor.
 */
static void tp_example_read_task(void *pvParameter)
{
    
    while(1) {       
        if (gpio_get_level(TOUCH_PIN1) == 0)
        {  
            printf("1 touch\n");
        } 
        else if (gpio_get_level(TOUCH_PIN2) == 0)
        {  
            printf("2 touch\n");
        } 
        else if (gpio_get_level(TOUCH_PIN3) == 0)
        {  
            printf("3 touch\n");
        } 
        else if (gpio_get_level(TOUCH_PIN4) == 0)
        {  
            printf("4 touch\n");
        }

        gpio_set_level(LED_1, 0);
        gpio_set_level(LED_2, 0);
        gpio_set_level(LED_3, 0);
        gpio_set_level(LED_4, 0);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        gpio_set_level(LED_1, 1);
        gpio_set_level(LED_2, 1);
        gpio_set_level(LED_3, 1);
        gpio_set_level(LED_4, 1);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

static void toggle_led(int led_pin)
{
// Get the current state of the LED
    bool current_state = get_led_state(led_pin);

    // Toggle the state
    set_led_state(led_pin, !current_state);
}

// Task to continuously check button presses and toggle LEDs
static void button_task(void *pvParameter)
{
    while (1)
    {
        if (xSemaphoreTake(xMutex, portMAX_DELAY))
        {
            // Toggle LED 1 based on button 1
            if (gpio_get_level(TOUCH_PIN1) == 0)
            {
                printf("1 touch\n");
                toggle_led(LED_1);
                vTaskDelay(LONG_PRESS_DURATION);
            }

            // Toggle LED 2 based on button 2
            if (gpio_get_level(TOUCH_PIN2) == 0)
            {
                printf("2 touch\n");
                toggle_led(LED_2);
                vTaskDelay(LONG_PRESS_DURATION);
            }

            // Toggle LED 3 based on button 3
            if (gpio_get_level(TOUCH_PIN3) == 0)
            {
                printf("3 touch\n");
                toggle_led(LED_3);
                vTaskDelay(LONG_PRESS_DURATION);
            }

            // Toggle LED 4 based on button 4
            if (gpio_get_level(TOUCH_PIN4) == 0)
            {
                printf("4 touch\n");
                toggle_led(LED_4);
                vTaskDelay(LONG_PRESS_DURATION);
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
    
    /* Start task to read values by pads. */
    //xTaskCreate(&tp_example_read_task, "touch_pad_read_task", 4096, NULL, 5, NULL);
    xTaskCreate(&button_task, "button_task", 4096, NULL, 5, NULL);
} 