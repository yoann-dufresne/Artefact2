#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <lwip/ip4_addr.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <esp_system.h>
#include <esp_timer.h>


#include "wifi.h"
#include "socket.h"


static const char *TAG = "wifi_gateway";



// -------- Main function --------

void app_main(void) {
    // Start the wifi ap
    wifi_init_softap();

    TaskHandle_t server = NULL;

    while(1) {
        // Start the tcp server
        if (server == NULL)
            xTaskCreate(tcp_server_task, "tcp_server_task", 4096, NULL, 5, &server);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
