#include <Arduino.h>

#include "panel.hpp"


#define HARDWARE_DEBUG true
void hardware_loop();


Panel * p;

void setup() {
  Serial.begin(115200);
  delay(100);
  p = new Panel(0);

  // Debug
  if(HARDWARE_DEBUG) { while (true) hardware_loop(); }
}

void loop() {

}


bool light[9] = {false, false, false, false, false, false, false, false, false};
void hardware_loop() {
  p->check_buttons();

  // 8 buttons
  for (int idx(0) ; idx<8 ; idx++)
  {
    if (p->buttons_status[idx] != light[idx])
    {
      p->set_color(idx, p->buttons_status[idx] ? 1 : 0);
      light[idx] = p->buttons_status[idx];
    }
    Serial.print(p->buttons_status[idx]);Serial.print(" ");
  }

  // Central button
  if (p->buttons_status[8] != light[8])
  {
    p->set_color(0, p->buttons_status[8] ? 1 : 0);
    light[8] = p->buttons_status[8];
  }
  Serial.print(p->buttons_status[8]);

  p->show_colors();
  Serial.println();

  delay(10);
}