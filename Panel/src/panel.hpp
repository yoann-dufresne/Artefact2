#include <Arduino.h>
#include "colors.h"
#include "FastLED.h"


#ifndef PANEL_H
#define PANEL_H


class Panel
{
private:
    const int buttons_pins[8] = {2, 0, 4, 16, 17, 5, 18, 19};
    const int central_pin = 23;
    const int central_led_pin = 12;

    int panel_idx;

    static const unsigned long bounce_delay = 30;
    unsigned long last_check[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    
    bool central_status = false;

    #define leds_pin 21
    CRGBArray<8> leds;

public:
    bool buttons_status[9];

    Panel(int panel_idx) : panel_idx(panel_idx)
    {
        // Init buttons
        for (int button_idx(0) ; button_idx<8 ; button_idx++)
        {
            buttons_status[button_idx] = false;
            int pin = buttons_pins[button_idx];
            pinMode(pin, INPUT_PULLUP);
        }
        pinMode(central_pin, INPUT_PULLUP);

        // Init leds
        pinMode(central_led_pin, OUTPUT);
        FastLED.addLeds<NEOPIXEL, leds_pin>(leds, 8);
        for (int idx(0) ; idx<8 ; idx++)
        {
            leds[idx].setRGB(colors[0][0], colors[0][1], colors[0][2]);
        }
        FastLED.show();
    };

    ~Panel() {};

    void check_buttons()
    {
        for (int button_idx(0) ; button_idx<9 ; button_idx++)
        {
            int pin = (button_idx == 8) ? this->central_pin : buttons_pins[button_idx];
            bool button_status = digitalRead(pin) == LOW;
            unsigned long t = millis();

            // Is it the same value than the privious one
            if (this->buttons_status[button_idx] == button_status)
            {
                this->last_check[button_idx] = t;
            }
            // Is the anti-bouncing delay over ?
            else if (t - this->last_check[button_idx] >= bounce_delay)
            {
                this->buttons_status[button_idx] = button_status;
                this->last_check[button_idx] = t;
            }
        }
    };

    void set_color(int led_idx, int color_idx)
    {
        this->leds[led_idx].setRGB(
            colors[color_idx][0],
            colors[color_idx][1],
            colors[color_idx][2]
        );
    }

    void show_colors()
    {
        FastLED.show();
    }

    void central_led(bool power)
    {
        digitalWrite(central_led_pin, power);
    }

    void test_leds()
    {
        for (int idx(0) ; idx<8 ; idx++)
            leds[idx].setRGB(30, 0, 0);
        FastLED.show(); delay(500);

        for (int idx(0) ; idx<8 ; idx++)
            leds[idx].setRGB(0, 30, 0);
        FastLED.show(); delay(500);

        for (int idx(0) ; idx<8 ; idx++)
            leds[idx].setRGB(0, 0, 30);
        FastLED.show(); delay(500);

        for (int idx(0) ; idx<8 ; idx++)
            leds[idx].setRGB(30, 30, 0);
        FastLED.show(); delay(500);

        for (int idx(0) ; idx<8 ; idx++)
            leds[idx].setRGB(30, 0, 30);
        FastLED.show(); delay(500);

        for (int idx(0) ; idx<8 ; idx++)
            leds[idx].setRGB(0, 30, 30);
        FastLED.show(); delay(500);

        for (int idx(0) ; idx<8 ; idx++)
            leds[idx].setRGB(30, 30, 30);
        FastLED.show(); delay(500);

        for (int idx(0) ; idx<8 ; idx++)
            leds[idx].setRGB(0, 0, 0);
        FastLED.show(); delay(500);
    };
};

#endif
