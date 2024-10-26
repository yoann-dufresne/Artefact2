#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <esp_log.h>
// #include <esp_log.h>
// #include <esp_netif.h>
// #include <lwip/ip4_addr.h>
// #include <lwip/sockets.h>
// #include <lwip/netdb.h>
// #include <esp_system.h>
// #include <esp_timer.h>
#include <freertos/FreeRTOS.h>
// #include <freertos/event_groups.h>

#include "wifi.h"
#include "sockets.h"
#include "leds.h"
#include "buttons.h"

#define PORT 8080
static const char *TAG = "panel";



// ------------------- MAIN -------------------

void app_main(void) {
    // Initialisation de la connexion au réseau
    wifi_init_sta();

    // Initialisation des rubans de leds
    init_led_strip();
    
    // Initialisation des boutons
    init_buttons();

    // Initialisation de la connexion permanente
    first_connection(NULL);


    while (1)
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
