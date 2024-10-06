#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

#define PORT 4242
#define MAX_CLIENTS 5

static const char *TAG = "server";
static int clients[MAX_CLIENTS];
static SemaphoreHandle_t clients_mutex;

void handle_client(void *pvParameters) {
    int client_socket = (int)pvParameters;
    char rx_buffer[128];
    char addr_str[128];
    int len;

    while ((len = recv(client_socket, rx_buffer, sizeof(rx_buffer) - 1, 0)) > 0) {
        rx_buffer[len] = 0; // Null-terminate the received data
        ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);
        if (strncmp(rx_buffer, "pong", 4) == 0) {
            ESP_LOGI(TAG, "Pong received from client");
        }
    }

    if (len < 0) {
        ESP_LOGE(TAG, "recv failed: errno %d", errno);
    } else if (len == 0) {
        ESP_LOGI(TAG, "Connection closed");
    }

    close(client_socket);
    xSemaphoreTake(clients_mutex, portMAX_DELAY);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == client_socket) {
            clients[i] = -1;
            break;
        }
    }
    xSemaphoreGive(clients_mutex);
    vTaskDelete(NULL);
}

void broadcast_ping(void *pvParameters) {
    while (1) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        char ping_message[64];
        snprintf(ping_message, sizeof(ping_message), "ping %ld", time(NULL));
        xSemaphoreTake(clients_mutex, portMAX_DELAY);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] != -1) {
                send(clients[i], ping_message, strlen(ping_message), 0);
            }
        }
        xSemaphoreGive(clients_mutex);
    }
}

void start_server(void *pvParameters) {
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int listen_socket, client_socket;

    listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listen_socket < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (bind(listen_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        close(listen_socket);
        vTaskDelete(NULL);
        return;
    }

    if (listen(listen_socket, MAX_CLIENTS) < 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        close(listen_socket);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Server listening on port %d", PORT);

    while (1) {
        client_socket = accept(listen_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            continue;
        }

        xSemaphoreTake(clients_mutex, portMAX_DELAY);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] == -1) {
                clients[i] = client_socket;
                xTaskCreate(handle_client, "handle_client", 4096, (void *)client_socket, 5, NULL);
                break;
            }
        }
        xSemaphoreGive(clients_mutex);
    }
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    clients_mutex = xSemaphoreCreateMutex();
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = -1;
    }

    xTaskCreate(start_server, "start_server", 4096, NULL, 5, NULL);
    xTaskCreate(broadcast_ping, "broadcast_ping", 4096, NULL, 5, NULL);
}