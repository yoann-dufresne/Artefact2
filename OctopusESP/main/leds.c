#include <esp_log.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include <rom/ets_sys.h>
#include "esp_rom_gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "leds";

#define NUM_STRIPS 8
#define NUM_LEDS_PER_STRIP 4

static uint8_t led_strip_pins[NUM_STRIPS] = {15, 2, 18, 19, 32, 25, 14, 12};
static uint8_t led_data[NUM_STRIPS][NUM_LEDS_PER_STRIP * 3];
static bool led_update[NUM_STRIPS][NUM_LEDS_PER_STRIP];

void set_led_color(uint8_t *data, bool* update, uint8_t r, uint8_t g, uint8_t b);

void init_led_strips() {
    // Initialisation des pins pour chaque ruban
    for (int i = 0; i < NUM_STRIPS; i++) {
        ESP_LOGI("LEDs", "Initialisation des LEDs sur la broche %d", led_strip_pins[i]);
        gpio_set_direction(led_strip_pins[i], GPIO_MODE_OUTPUT);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "LED strips initialized.");
}

void set_led_color(uint8_t *data, bool* update, uint8_t r, uint8_t g, uint8_t b) {
    if (data[0] == r && data[1] == g && data[2] == b) {
        return;
    }

    data[0] = g;
    data[1] = r;
    data[2] = b;
    *update = true;
}

void set_led_state(int strip_num, int led_num, uint8_t r, uint8_t g, uint8_t b) {
    if (strip_num >= 0 && strip_num < NUM_STRIPS && led_num >= 0 && led_num < NUM_LEDS_PER_STRIP) {
        set_led_color(&led_data[strip_num][led_num * 3], &led_update[strip_num][led_num], r, g, b);
    }
}


void send_bit_1(int pin) {
    gpio_set_level(pin, 1);
    ets_delay_us(0.7);  // ~0.7 µs pour "HIGH" de bit 1
    gpio_set_level(pin, 0);
    ets_delay_us(0.6);  // ~0.6 µs pour "LOW" de bit 1
}

void send_bit_0(int pin) {
    gpio_set_level(pin, 1);
    ets_delay_us(0.35);  // ~0.35 µs pour "HIGH" de bit 0
    gpio_set_level(pin, 0);
    ets_delay_us(0.8);   // ~0.8 µs pour "LOW" de bit 0
}

void refresh_led_strip(int strip_num) {
    ESP_LOGI(TAG, "Refreshing LED strip %d", strip_num);
    
    for (int i = 0; i < NUM_LEDS_PER_STRIP * 3; i++) {
        uint8_t byte = led_data[strip_num][i];
        for (int i = 0; i < 8; i++) {
            if (byte & (1 << (7 - i))) {
                send_bit_1(led_strip_pins[strip_num]);
            } else {
                send_bit_0(led_strip_pins[strip_num]);
            }
        }
    }
    ets_delay_us(80);
}



void get_color(char c, uint8_t* r, uint8_t* g, uint8_t* b) {
    static const int intensity = 10;
    switch (c) {
        case 'n': *r = 0; *g = 0; *b = 0; break;
        case 'v': *r = 0; *g = intensity; *b = 0; break;
        case 'r': *r = intensity; *g = 0; *b = 0; break;
        case 'l': *r = 0; *g = 0; *b = intensity; break;
        case 'j': *r = intensity; *g = intensity; *b = 0; break;
        case 'm': *r = intensity; *g = 0; *b = intensity; break;
        case 't': *r = 0; *g = intensity*2; *b = intensity; break;
        case 'o': *r = intensity*2.5; *g = intensity/2; *b = 0; break;
        case 'b': *r = intensity; *g = intensity; *b = intensity; break;
        default: *r = 0; *g = 0; *b = 0; break;
    }
}


void update_led_strip_with_array(int strip_num, char color_array[32]) {
    ESP_LOGI(TAG, "update leds %d: %32s", strip_num, color_array);

    if (strip_num >= 0 && strip_num < NUM_STRIPS) {
        for (int i = 0; i < 32; i++) {
            uint8_t r = 0, g = 0, b = 0;
            get_color(color_array[i], &r, &g, &b);
            set_led_state(strip_num, i * 2, r, g, b);
            set_led_state(strip_num, i * 2 + 1, r, g, b);
        }
        refresh_led_strip(strip_num);
    }
}