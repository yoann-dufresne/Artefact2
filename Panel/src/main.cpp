#include <Arduino.h>

#include "panel.hpp"
#include "Network.hpp"


#define HARDWARE_DEBUG false
void hardware_loop();
void network_loop();


Panel * p;
Network * net;

void setup() {
  Serial.begin(115200);
  delay(3000);
  p = new Panel(0);

  // Debug
  if(HARDWARE_DEBUG) { while (true) hardware_loop(); }
  
  net = new Network();
  net->connect_network();
  while (not net->connect_server(1)) {
    delay(5000);
  };

  while (true) network_loop();
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


void network_loop() {
  p->check_buttons();
  uint8_t msg[3] = {'B', '9', '0'};

  // 8 buttons
  for (int idx(0) ; idx<8 ; idx++)
  {
    if (p->buttons_status[idx] != light[idx])
    {
      p->set_color(idx, p->buttons_status[idx] ? 1 : 0);
      light[idx] = p->buttons_status[idx];

      msg[1] = '0' + idx;
      msg[2] = p->buttons_status[idx] ? '1' : '0';
      net->send(msg, 3);
    }
  }

  // Central button
  if (p->buttons_status[8] != light[8])
  {
    p->set_color(0, p->buttons_status[8] ? 1 : 0);
    light[8] = p->buttons_status[8];
    msg[1] = '8';
    msg[2] = p->buttons_status[8] ? '1' : '0';
    net->send(msg, 3);
  }

  p->show_colors();

  delay(10);
}


void loop() {};