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

    int panel_idx;

    static const int num_checks = 5;
    bool buttons_checks[8][num_checks];
    int buttons_sums[8];
    int check_idx;
    bool central_status = false;

    #define leds_pin 21
    CRGBArray<8> leds;

public:
    Panel(int panel_idx) : panel_idx(panel_idx), check_idx(0)
    {
        // Init buttons
        for (int button_idx(0) ; button_idx<8 ; button_idx++)
        {
            int pin = buttons_pins[button_idx];
            pinMode(pin, INPUT_PULLUP);
            buttons_sums[button_idx] = 0;
            for (int i(0) ; i<num_checks ; i++)
                buttons_checks[button_idx][i] = false;
            
        }
        pinMode(central_pin, INPUT_PULLUP);

        // Init leds
        FastLED.addLeds<NEOPIXEL, leds_pin>(leds, 8);
        for (int idx(0) ; idx<8 ; idx++)
        {
            leds[idx].setRGB(colors[0][0], colors[0][1], colors[0][2]);
        }
        FastLED.show();
    };

    ~Panel() {};

    void check_buttons(bool status[8])
    {
        for (int button_idx(0) ; button_idx<8 ; button_idx++)
        {
            int pin = buttons_pins[button_idx];
            bool button_status = buttons_checks[button_idx][check_idx];
            buttons_sums[button_idx] -= button_status ? 1 : 0;

            button_status = digitalRead(pin) == HIGH;

            buttons_checks[button_idx][check_idx] = button_status;
            buttons_sums[button_idx] += button_status ? 1 : 0;

            status[button_idx] = buttons_sums[button_idx] > (num_checks / 2);
        }

        check_idx = (check_idx + 1) % num_checks;
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
