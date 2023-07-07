#include <Arduino.h>

#include "octopus.hpp"
#include "Network.hpp"


#define HARDWARE_DEBUG false
void hardware_loop();
void network_loop();


Octopus * octo;
Network * net;

void setup() {
  Serial.begin(115200);
  delay(3000);
  octo = new Octopus();

  // Debug
  if(HARDWARE_DEBUG) { while (true) hardware_loop(); }
  
  net = new Network();

  while (true) network_loop();
}



bool light[9] = {false, false, false, false, false, false, false, false, false};
void hardware_loop() {
  

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
  
  bool light = false;

  // Play animation
  if (octo->current_animation)
  {
    octo->fade_out();
    light = true;
  }

  // Get messages from server
  uint8_t * rcv;
  while ((rcv = net->read_message(10)) != nullptr)
  {
    printf("Message received\n");
    int num_colors, i, color_idx, strip_idx, led_idx, animation;
    switch (static_cast<char>(rcv[0]))
    {
    case 'C': // Change color map
      num_colors = net->msg_size / 3;
      octo->change_color_map(num_colors, rcv+1);
      break;

    case 'L': // Light the panel leds (all 9)
      light = true;
      strip_idx = rcv[1];
      animation = rcv[2];

      if (animation == 1) // fade out
      {
        printf("Fade out\n");
        octo->init_animation();
        break;
      }

      printf("Light %d\n", strip_idx);

      num_colors = net->msg_size-3;
      for (led_idx=0 ; led_idx<64 ; led_idx++)
      {
        i = num_colors * led_idx / 64;
        color_idx = rcv[3+i];
        printf("%d(%d) ", led_idx, color_idx);
        octo->set_color(strip_idx, led_idx, color_idx);
      }
      printf("\n");
      break;

    default:
      break;
    }
  }

  if (light)
    octo->show_colors();
}


void loop() {};