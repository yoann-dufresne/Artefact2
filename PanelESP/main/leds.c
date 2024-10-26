#include "led_strip.h"
#include <esp_log.h>

#include "leds.h"

#define NUM_LEDS 8

#define STRIP_PIN 21
#define SWAG_LED_PIN 23
// #define SWAG_BTN_PIN 12

static const char *TAG = "led";

static uint8_t led_data[NUM_LEDS * 3];
static led_strip_handle_t led_strip;



void set_led_color(int led_idx, uint8_t r, uint8_t g, uint8_t b);
void refresh_led_strip();



void init_led_strip() {
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = STRIP_PIN, // The GPIO that connected to the LED strip's data line
        .max_leds = NUM_LEDS,      // The number of LEDs in the strip,
        .led_model = LED_MODEL_WS2812,        // LED strip model
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // The color order of the strip: GRB
        .flags = {
            .invert_out = false, // don't invert the output signal
        }
    };

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
        .resolution_hz = 10 * 1000 * 1000, // RMT counter clock frequency
        .mem_block_symbols = 64,               // the memory size of each RMT channel, in words (4 bytes)
        .flags = {
            .with_dma = false, // DMA feature is available on chips like ESP32-S3/P4
        }
    };

    // Setup the physical device
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &(led_strip)));

    // Setup the data buffer
    for (int i = 0; i < NUM_LEDS; i++) {
        set_led_color(i, 0, 0, 0);
    }
    
    ESP_LOGI(TAG, "LED strip initialized.");
}

void set_led_color(int led_idx, uint8_t r, uint8_t g, uint8_t b) {
    led_data[led_idx * 3    ] = r;
    led_data[led_idx * 3 + 1] = g;
    led_data[led_idx * 3 + 2] = b;
}

void refresh_led_strip() {
    ESP_LOGI(TAG, "Refreshing LED strip");
    // Update the leds with the new data
    for (int led_idx=0 ; led_idx<NUM_LEDS ; led_idx++) {
        led_strip_set_pixel(led_strip, led_idx, led_data[led_idx*3], led_data[led_idx*3+1], led_data[led_idx*3+2]);
    }
    // Refrech the led strip physically
    led_strip_refresh(led_strip);
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


void update_led_strip_with_array(char color_array[NUM_LEDS+1]) {
    ESP_LOGI(TAG, "update leds: %9s", color_array);

    for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t r = 0, g = 0, b = 0;
        get_color(color_array[i], &r, &g, &b);
        set_led_color(i, r, g, b);
    }
    refresh_led_strip();

    // TODO: Faire la lumière du bouton central
}