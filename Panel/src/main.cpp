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

  // Reconnect network/server
  while (not net->is_connected_to_server())
  {
    // Wifi connection
    if (not net->is_connected_on_wifi())
      net->connect_network();
    // server connection
    if (not net->connect_server(1)) {
      delay(5000);
    };
  }


  p->check_buttons();
  uint8_t msg[3] = {'B', '9', '0'};

  // 8 buttons
  for (int idx(0) ; idx<8 ; idx++)
  {
    if (p->buttons_status[idx] != light[idx])
    {
      // p->set_color(idx, p->buttons_status[idx] ? 1 : 0);
      light[idx] = p->buttons_status[idx];

      msg[1] = idx;
      msg[2] = p->buttons_status[idx] ? 1 : 0;
      net->send(msg, 3);
    }
  }

  // Central button
  if (p->buttons_status[8] != light[8])
  {
    // p->set_color(0, p->buttons_status[8] ? 1 : 0);
    light[8] = p->buttons_status[8];
    msg[1] = 8;
    msg[2] = p->buttons_status[8] ? 1 : 0;
    net->send(msg, 3);
  }

  // Get messages from server
  uint8_t * rcv = net->read_message(10);
  if (rcv != nullptr)
  {
    printf("Message received\n");
    int num_colors, i, color_idx;
    switch (static_cast<char>(rcv[0]))
    {
    case 'C': // Change color map
      num_colors = net->msg_size / 3;
      p->change_color_map(num_colors, rcv+1);
      break;

    case 'L': // Light the panel leds (all 9)
      printf("Light\n");
      for (i=0 ; i<8 ; i++)
      {
        color_idx = rcv[1+i];
        printf("%d(%d) ", i, color_idx);
        p->set_color(i, color_idx);
      }
      printf("\n");

      p->central_led(rcv[9] != 0);
      p->show_colors();
      break;

    default:
      break;
    }
  }
}


void loop() {};