
#ifndef LEDS_H
#define LEDS_H

void init_led_strips();
void update_led_strip_with_array(int strip_num, char color_array[32]);

#endif // LEDS_H