#include <stdio.h>
#include <esp_log.h>
#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define NUM_LEDS 64
#define NUM_STRIPS 8
// static uint8_t led_strip_pins[NUM_STRIPS] = {15, 2, 18, 19, 32, 25, 14, 12};
static uint8_t led_strip_pins[NUM_STRIPS] = {1, 4, 13, 20, 40, 15, 0, 46};

// Prototypes
void send_byte(uint8_t byte, gpio_num_t pin);
void send_led_data(uint8_t *data, int num_leds, gpio_num_t pin);
void set_color(uint8_t *data, int index, uint8_t red, uint8_t green, uint8_t blue);

void app_main(void) {
    // Initialisation des pins pour chaque ruban
    for (int i = 0; i < NUM_STRIPS; i++) {
        ESP_LOGI("LEDs", "Initialisation des LEDs sur la broche %d", led_strip_pins[i]);
        gpio_set_direction(led_strip_pins[i], GPIO_MODE_OUTPUT);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_LOGI("LEDs", "Initialisation des LEDs terminée");

    while (1) {
        for (int strip = 0; strip < NUM_STRIPS; strip++) {
            uint8_t led_data[NUM_LEDS * 3];  // 3 bytes par LED (RGB)
            for (int i = 0; i < NUM_LEDS; i++) {
                set_color(led_data, i, 255, 0, 0);  // Mettre toutes les LEDs au rouge
            }
            send_led_data(led_data, NUM_LEDS, led_strip_pins[strip]);
        }
        vTaskDelay(pdMS_TO_TICKS(500));

        for (int strip = 0; strip < NUM_STRIPS; strip++) {
            uint8_t led_data[NUM_LEDS * 3] = {0};  // Toutes les LEDs éteintes
            send_led_data(led_data, NUM_LEDS, led_strip_pins[strip]);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void set_color(uint8_t *data, int index, uint8_t red, uint8_t green, uint8_t blue) {
    data[index * 3] = green;   // WS2812 utilise le format GRB
    data[index * 3 + 1] = red;
    data[index * 3 + 2] = blue;
}

void send_led_data(uint8_t *data, int num_leds, gpio_num_t pin) {
    for (int i = 0; i < num_leds * 3; i++) {
        send_byte(data[i], pin);
    }
}

void send_byte(uint8_t byte, gpio_num_t pin) {
    for (int i = 0; i < 8; i++) {
        if (byte & (1 << (7 - i))) {
            gpio_set_level(pin, 1);
            esp_rom_delay_us(1);  // Durée plus courte
            gpio_set_level(pin, 0);
            esp_rom_delay_us(2);  // Durée plus longue
        } else {
            gpio_set_level(pin, 1);
            esp_rom_delay_us(2);  // Durée plus longue
            gpio_set_level(pin, 0);
            esp_rom_delay_us(1);  // Durée plus courte
        }
    }
}
