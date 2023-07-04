#include <Arduino.h>
#include "colors.h"
#include "FastLED.h"


#ifndef OCTOPUS_H
#define OCTOPUS_H


const int strips_pins[8] = {15, 2, 18, 19, 32, 25, 14, 12};

class Octopus
{
private:
    CRGB ** leds;

    int num_colors;
    int ** colors;
    
public:
    bool buttons_status[9];

    Octopus()
    {
        // Init leds
        this->leds = new CRGB*[8];
        for (int strip=0 ; strip<8 ; strip++)
        {
            this->leds[strip] = new CRGB[64];
            // for (int led=0 ; led<64 ; led++)
            // {

            // }
        }

        // Init colors
        this->num_colors = num_default_colors;
        this->colors = new int*[num_default_colors];
        for (size_t i=0 ; i<num_default_colors ; i++)
        {
            this->colors[i] = new int[3];
            for (size_t c=0 ; c<3 ; c++)
                this->colors[i][c] = default_colors[i][c];
        }

        // Init led pins
        FastLED.addLeds<NEOPIXEL, 15>(leds[0], 64);
        FastLED.addLeds<NEOPIXEL, 2>(leds[1], 64);
        FastLED.addLeds<NEOPIXEL, 18>(leds[2], 64);
        FastLED.addLeds<NEOPIXEL, 19>(leds[3], 64);
        FastLED.addLeds<NEOPIXEL, 32>(leds[4], 64);
        FastLED.addLeds<NEOPIXEL, 25>(leds[5], 64);
        FastLED.addLeds<NEOPIXEL, 14>(leds[6], 64);
        FastLED.addLeds<NEOPIXEL, 12>(leds[7], 64);

        // Init leds
        for (uint i=0 ; i<8 ; i++)
        {
            pinMode(strips_pins[i], OUTPUT);
            for (int led_idx(0) ; led_idx<64 ; led_idx++)
            {
                leds[i][led_idx].setRGB(colors[0][0], colors[0][1], colors[0][2]);
            }
            FastLED.show();
        }
    };

    ~Octopus() {
        // Leds
        for (int i=0 ; i<8 ; i++)
            delete[] this->leds[i];
        delete[] this->leds;

        // Colors
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

    void set_color(int strip_idx, int led_idx, int color_idx)
    {
        // printf("set_color %d %d %d\n", strip_idx, led_idx, color_idx);
        this->leds[strip_idx][led_idx].setRGB(
            this->colors[color_idx][0],
            this->colors[color_idx][1],
            this->colors[color_idx][2]
        );
    }

    void fade_out()
    {
        int n = 15;
        for (int i(0) ; i<n ; i++)
        {
            for (int strip=0 ; strip<8 ; strip++)
            {
                for (int led=0 ; led<64 ; led++)
                {
                    int r = this->leds[strip][led].r;
                    int dr = r / (n - 1 - i);
                    int g = this->leds[strip][led].g;
                    int dg = g / (n - 1 - i);
                    int b = this->leds[strip][led].b;
                    int db = b - b / (n - 1 - i);

                    this->leds[strip][led].setRGB(r-dr, g-dg, b-db);
                }
            }

            FastLED.show();
            delay(1500 / n);
        }
    }

    void show_colors()
    {
        FastLED.show();
    }

    void test_leds()
    {
        
    };
};

#endif
