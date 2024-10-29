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
    xTaskCreate(&init_connection, "init_connection", 2048, NULL, 5, NULL);

    while (1)
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        set_led_state(5, 0, 50, 0, 0);
        refresh_led_strip(5);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        set_led_state(5, 0, 0, 0, 0);
        refresh_led_strip(5); 
    }
}
