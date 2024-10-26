#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <esp_log.h>
// #include <esp_log.h>
// #include <esp_netif.h>
// #include <lwip/ip4_addr.h>
// #include <lwip/sockets.h>
// #include <lwip/netdb.h>
// #include <esp_system.h>
// #include <esp_timer.h>
#include <freertos/FreeRTOS.h>
// #include <freertos/event_groups.h>

#include "wifi.h"
#include "sockets.h"

#define PORT 8080
static const char *TAG = "panel";

#define BTN0_GPIO 19
// #define BTN1_GPIO 18
// #define BTN2_GPIO 5
// #define BTN3_GPIO 17
// #define BTN4_GPIO 16
// #define BTN5_GPIO 4
// #define BTN6_GPIO 0
// #define BTN7_GPIO 2
// #define CENTRAL_BTN 23

// #include "led_strip.h"
// #define NUM_LEDS 8

// static uint8_t led_strip_pins[NUM_STRIPS] = {15, 2, 18, 19, 32, 25, 14, 12};
// static uint8_t led_data[NUM_STRIPS][NUM_LEDS_PER_STRIP * 3];

// static led_strip_handle_t led_strip[NUM_STRIPS];

// void set_led_color(uint8_t *data, uint8_t r, uint8_t g, uint8_t b);

// void init_led_strips() {
//     /* LED strip initialization with the GPIO and pixels number*/
//     led_strip_config_t strip_config = {
//         .strip_gpio_num = 15, // The GPIO that connected to the LED strip's data line
//         .max_leds = NUM_LEDS_PER_STRIP, // The number of LEDs in the strip,
//         .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
//         .led_model = LED_MODEL_WS2812, // LED strip model
//         .flags.invert_out = false, // whether to invert the output signal (useful when your hardware has a level inverter)
//     };

//     led_strip_rmt_config_t rmt_config = {
//         .clk_src = RMT_CLK_SRC_DEFAULT, // different clock source can lead to different power consumption
//         .resolution_hz = 10 * 1000 * 1000, // 10MHz
//         .flags.with_dma = false, // whether to enable the DMA feature
//     };

//     for (int i = 0; i < NUM_STRIPS; i++) {
//         // Setup the phisical device
//         strip_config.strip_gpio_num = led_strip_pins[i];
//         ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &(led_strip[i])));

//         // Setup the data buffer
//         for (int j = 0; j < NUM_LEDS_PER_STRIP; j++) {
//             set_led_color(&led_data[i][j * 3], 0, 0, 0);
//         }
//     }
    
//     ESP_LOGI(TAG, "LED strips initialized.");
// }

// void set_led_color(uint8_t *data, uint8_t r, uint8_t g, uint8_t b) {
//     data[0] = r;
//     data[1] = g;
//     data[2] = b;
// }

// void set_led_state(int strip_num, int led_num, uint8_t r, uint8_t g, uint8_t b) {
//     if (strip_num >= 0 && strip_num < NUM_STRIPS && led_num >= 0 && led_num < NUM_LEDS_PER_STRIP) {
//         set_led_color(&led_data[strip_num][led_num * 3], r, g, b);
//     }
// }

// size_t encode_led_data(uint8_t* led_data, rmt_symbol_word_t* symbols, size_t led_count) {
//     size_t symbol_idx = 0;
//     for (size_t i = 0; i < led_count; i++) {
//         for (int bit = 7; bit >= 0; bit--) {
//             symbols[symbol_idx].level0 = 1;
//             symbols[symbol_idx].duration0 = (led_data[i] & (1 << bit)) ? 8 : 4;
//             symbols[symbol_idx].level1 = 0;
//             symbols[symbol_idx].duration1 = (led_data[i] & (1 << bit)) ? 4 : 8;
//             symbol_idx++;
//         }
//     }
//     return symbol_idx;
// }

// void refresh_led_strip(int strip_num) {
//     ESP_LOGI(TAG, "Refreshing LED strip %d", strip_num);
//     // Update the leds with the new data
//     for (int led_idx=0 ; led_idx<NUM_LEDS_PER_STRIP ; led_idx++) {
//         led_strip_set_pixel(led_strip[strip_num], led_idx, led_data[strip_num][led_idx*3], led_data[strip_num][led_idx*3+1], led_data[strip_num][led_idx*3+2]);
//     }
//     // Refrech the led strip physically
//     led_strip_refresh(led_strip[strip_num]);
// }


// void get_color(char c, uint8_t* r, uint8_t* g, uint8_t* b) {
//     static const int intensity = 10;
//     switch (c) {
//         case 'n': *r = 0; *g = 0; *b = 0; break;
//         case 'v': *r = 0; *g = intensity; *b = 0; break;
//         case 'r': *r = intensity; *g = 0; *b = 0; break;
//         case 'l': *r = 0; *g = 0; *b = intensity; break;
//         case 'j': *r = intensity; *g = intensity; *b = 0; break;
//         case 'm': *r = intensity; *g = 0; *b = intensity; break;
//         case 't': *r = 0; *g = intensity*2; *b = intensity; break;
//         case 'o': *r = intensity*2.5; *g = intensity/2; *b = 0; break;
//         case 'b': *r = intensity; *g = intensity; *b = intensity; break;
//         default: *r = 0; *g = 0; *b = 0; break;
//     }
// }


// void update_led_strip_with_array(int strip_num, char color_array[32]) {
//     ESP_LOGI(TAG, "update leds %d: %32s", strip_num, color_array);

//     if (strip_num >= 0 && strip_num < NUM_STRIPS) {
//         for (int i = 0; i < 32; i++) {
//             uint8_t r = 0, g = 0, b = 0;
//             get_color(color_array[i], &r, &g, &b);
//             set_led_state(strip_num, i * 2, r, g, b);
//             set_led_state(strip_num, i * 2 + 1, r, g, b);
//         }
//         refresh_led_strip(strip_num);
//     }
// }





// ------------------- MAIN -------------------

void app_main(void) {
    // Initialisation de la connexion au réseau
    wifi_init_sta();
    first_connection(NULL);

    // // Initialisation des rubans de leds
    // init_led_strips();

    // // Initialisation du socket
    // xTaskCreate(socket_task, "socket_task", 4096, NULL, 5, NULL);

    while (1)
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
