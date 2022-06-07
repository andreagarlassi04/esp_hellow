/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"

static const char *TAG = "example";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO4 CONFIG_BLINK_GPIO

static uint8_t s_ledB_state = 0;

#ifdef CONFIG_BLINK_LED_GPIO

static void blink_ledB(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(4, s_ledB_state);
}

static void configure_ledB(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(4);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(4, GPIO_MODE_OUTPUT);
}

#endif

#define BLINK_GPIO0 CONFIG_BLINK_GPIO

static uint8_t s_ledR_state = 0;

#ifdef CONFIG_BLINK_LED_GPIO

static void blink_ledR(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(0, s_ledR_state);
}

static void configure_ledR(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(0);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(0, GPIO_MODE_OUTPUT);
}

#endif

#define BLINK_GPIO2 CONFIG_BLINK_GPIO

static uint8_t s_ledG_state = 0;

#ifdef CONFIG_BLINK_LED_GPIO

static void blink_ledG(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(2, s_ledG_state);
}

static void configure_ledG(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(2);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(2, GPIO_MODE_OUTPUT);
}

#endif

void app_main(void)
{

    /* Configure the peripheral according to the LED type */
    configure_ledB();
    configure_ledR();
    configure_ledG();

    while (1) {
        ESP_LOGI(TAG, "Turning the LED %s!", s_ledB_state == true ? "ON" : "OFF");
        blink_ledB();
        /* Toggle the LED state */
        s_ledB_state = !s_ledB_state;
        printf("Hello world!\n");
        for (int i = 3; i > 0; i--) {
            printf("Restarting in %d seconds...\n", i);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Turning the LED %s!", s_ledR_state == true ? "ON" : "OFF");
        blink_ledR();
        /* Toggle the LED state */
        s_ledR_state = !s_ledR_state;
        printf("Hello world!\n");
        for (int i = 3; i > 0; i--) {
            printf("Restarting in %d seconds...\n", i);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Turning the LED %s!", s_ledG_state == true ? "ON" : "OFF");
        blink_ledG();
        /* Toggle the LED state */
        s_ledG_state = !s_ledG_state;
        printf("Hello world!\n");
        for (int i = 3; i > 0; i--) {
            printf("Restarting in %d seconds...\n", i);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}
 


