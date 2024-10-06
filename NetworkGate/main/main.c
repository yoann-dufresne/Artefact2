#include "wifi.h"
#include "socket.h"

// #include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"


// static const char *TAG = "main";


void app_main(void)
{
    nvs_flash_init();
    wifi_init_softap(); // Démarre le point d'accès WiFi

    // Démarre la tâche du serveur TCP
    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
}
