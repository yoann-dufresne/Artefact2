#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include <freertos/FreeRTOS.h>

#include "wifi.h"
#include "socket.h"
#include "leds.h"

#define PORT 8080
// static const char *TAG = "octopus";




// ------------------- MAIN -------------------

void app_main(void) {
    // Initialisation de la connexion au réseau
    wifi_init_sta();

    // Initialisation des rubans de leds
    init_led_strips();

    // Initialisation du socket
    init_connection(NULL);

    while (1)
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
