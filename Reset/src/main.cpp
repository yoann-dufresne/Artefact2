#include <Arduino.h>

#include "Network.hpp"


#define HARDWARE_DEBUG false
void hardware_loop();
void network_loop();

Network * net;

void setup() {
  Serial.begin(115200);
  delay(3000);

  pinMode(17, INPUT_PULLUP);

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

  uint8_t msg[1] = {'R'};

  if (digitalRead(17) == LOW)
    net->send(msg, 1);
}


void loop() {};