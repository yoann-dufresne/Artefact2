#include <Arduino.h>
#include "colors.h"
#include "FastLED.h"


#ifndef PANEL_H
#define PANEL_H


class Panel
{
private:
    // const int buttons_pins[8] = {2, 0, 4, 16, 17, 5, 18, 19};
    const int buttons_pins[8] = {19, 18, 5, 17, 16, 4, 0, 2};
    const int central_pin = 23;
    const int central_led_pin = 12;

    int num_colors;
    int ** colors;

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
        // Init colors
        this->num_colors = num_default_colors;
        this->colors = new int*[num_default_colors];
        for (size_t i=0 ; i<num_default_colors ; i++)
        {
            this->colors[i] = new int[3];
            for (size_t c=0 ; c<3 ; c++)
                this->colors[i][c] = default_colors[i][c];
        }

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

    ~Panel() {
        for (int i=0 ; i<this->num_colors ; i++)
            delete[] this->colors[i];
        delete[] this->colors;
    };

    void change_color_map(int num_colors, uint8_t * colors)
    {
        // Remove previous colors
        for (int i=0 ; i<this->num_colors ; i++)
            delete[] this->colors[i];
        delete[] this->colors;

        // New colors
        this->num_colors = num_colors;
        this->colors = new int*[num_colors];
        for (int i=0 ; i<num_colors ; i++)
        {
            this->colors[i] = new int[3];
            for (int c=0 ; c<3 ; c++)
                this->colors[i][c] = static_cast<int>(colors[i*3 + c]);
        }
    }

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
            this->colors[color_idx][0],
            this->colors[color_idx][1],
            this->colors[color_idx][2]
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
