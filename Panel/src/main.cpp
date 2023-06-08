#include <Arduino.h>

#include "panel.hpp"

Panel p(0);
bool light[8] = {false, false, false, false, false, false, false, false};

void setup() {
  Serial.begin(115200);
}

void loop() {
  bool current_status[8];
  p.check_buttons(current_status);

  for (int idx(0) ; idx<8 ; idx++)
  {
    if (current_status[idx] != light[idx])
    {
      p.set_color(idx, current_status[idx] ? 1 : 0);
      light[idx] = current_status[idx];
    }
    Serial.print(current_status[idx]);Serial.print(" ");
  }
  p.show_colors();
  Serial.println();

  delay(10);
}