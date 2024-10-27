#include <esp_log.h>

#include "led_strip.h"

static const char *TAG = "leds";

#define NUM_STRIPS 8
#define NUM_LEDS_PER_STRIP 64

static uint8_t led_strip_pins[NUM_STRIPS] = {15, 2, 18, 19, 32, 25, 14, 12};
static uint8_t led_data[NUM_STRIPS][NUM_LEDS_PER_STRIP * 3];
static bool led_update[NUM_STRIPS][NUM_LEDS_PER_STRIP];

static led_strip_handle_t led_strip[NUM_STRIPS];

void set_led_color(uint8_t *data, bool* update, uint8_t r, uint8_t g, uint8_t b);

void init_led_strips() {
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = 15, // The GPIO that connected to the LED strip's data line
        .max_leds = NUM_LEDS_PER_STRIP, // The number of LEDs in the strip,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812, // LED strip model
        .flags.invert_out = false, // whether to invert the output signal (useful when your hardware has a level inverter)
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // different clock source can lead to different power consumption
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .mem_block_symbols = 64,
        .flags.with_dma = false, // whether to enable the DMA feature
    };

    for (int i = 0; i < NUM_STRIPS; i++) {
        // Setup the phisical device
        strip_config.strip_gpio_num = led_strip_pins[i];
        ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &(led_strip[i])));

        // Setup the data buffer
        for (int j = 0; j < NUM_LEDS_PER_STRIP; j++) {
            set_led_color(&led_data[i][j * 3], &led_update[i][j], 5, 5, 5);
        }
    }
    
    ESP_LOGI(TAG, "LED strips initialized.");
}

void set_led_color(uint8_t *data, bool* update, uint8_t r, uint8_t g, uint8_t b) {
    if (data[0] == r && data[1] == g && data[2] == b) {
        return;
    }

    data[0] = r;
    data[1] = g;
    data[2] = b;
    *update = true;
}

void set_led_state(int strip_num, int led_num, uint8_t r, uint8_t g, uint8_t b) {
    if (strip_num >= 0 && strip_num < NUM_STRIPS && led_num >= 0 && led_num < NUM_LEDS_PER_STRIP) {
        set_led_color(&led_data[strip_num][led_num * 3], &led_update[strip_num][led_num], r, g, b);
    }
}

size_t encode_led_data(uint8_t* led_data, rmt_symbol_word_t* symbols, size_t led_count) {
    size_t symbol_idx = 0;
    for (size_t i = 0; i < led_count; i++) {
        for (int bit = 7; bit >= 0; bit--) {
            symbols[symbol_idx].level0 = 1;
            symbols[symbol_idx].duration0 = (led_data[i] & (1 << bit)) ? 8 : 4;
            symbols[symbol_idx].level1 = 0;
            symbols[symbol_idx].duration1 = (led_data[i] & (1 << bit)) ? 4 : 8;
            symbol_idx++;
        }
    }
    return symbol_idx;
}

void refresh_led_strip(int strip_num) {
    ESP_LOGI(TAG, "Refreshing LED strip %d", strip_num);
    bool to_refrash = false;
    // Update the leds with the new data
    for (int led_idx=0 ; led_idx<NUM_LEDS_PER_STRIP ; led_idx++) {
        if (led_update[strip_num][led_idx]) {
            to_refrash = true;
            led_update[strip_num][led_idx] = false;
            led_strip_set_pixel(led_strip[strip_num], led_idx, led_data[strip_num][led_idx*3], led_data[strip_num][led_idx*3+1], led_data[strip_num][led_idx*3+2]);
        }
    }

    if (to_refrash) {    
        // Refrech the led strip physically
        led_strip_refresh(led_strip[strip_num]);
    }
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