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


bool light[8] = {false, false, false, false, false, false, false, false};
void hardware_loop() {
  bool current_status[8];
  p->check_buttons(current_status);

  for (int idx(0) ; idx<8 ; idx++)
  {
    if (current_status[idx] != light[idx])
    {
      p->set_color(idx, current_status[idx] ? 1 : 0);
      light[idx] = current_status[idx];
    }
    Serial.print(current_status[idx]);Serial.print(" ");
  }
  p->show_colors();
  Serial.println();

  delay(10);
}