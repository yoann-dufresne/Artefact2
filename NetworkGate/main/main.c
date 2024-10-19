#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <esp_system.h>

#define WIFI_SSID "artefact"
#define PORT 8080
static const char *TAG = "wifi_gateway";

void handle_device_communication(void *param);

static void start_wifi_ap() {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();
    // Configuration de l'AP Wi-Fi avec SSID
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .max_connection = 10,
            .password = "tcafetra",
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP started. SSID: %s", WIFI_SSID);
}

void handle_register(int client_socket, struct sockaddr_in client_addr) {
    char buffer[64];
    int len = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (len > 0) {
        buffer[len] = '\0';
        ESP_LOGI(TAG, "Received registration message: %s", buffer);

        // Analyse du message
        char name[33], type[16];
        if (sscanf(buffer, "register %32s %15s", name, type) == 2) {
            ESP_LOGI(TAG, "Device name: %s, Type: %s", name, type);

            // Assigner un port pour le client
            int client_port = PORT + 1;  // Ex: à ajuster selon les connexions
            char response[32];
            snprintf(response, sizeof(response), "port %d", client_port);

            send(client_socket, response, strlen(response), 0);
            ESP_LOGI(TAG, "Assigned port %d to device %s", client_port, name);

            // Fermer la socket après l'envoi du port
            close(client_socket);

            // Lancer une tâche pour gérer les communications avec ce device
            xTaskCreate(handle_device_communication, "device_task", 4096, (void *)client_port, 5, NULL);
        } else {
            ESP_LOGE(TAG, "Invalid registration message");
        }
    }
}

void start_server() {
    int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    struct sockaddr_in server_addr;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, 5);

    ESP_LOGI(TAG, "Server listening on port %d", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);

        ESP_LOGI(TAG, "New client connected");

        // Gérer le message d'enregistrement
        handle_register(client_socket, client_addr);
    }
}

void handle_device_communication(void *param) {
    int client_port = (int)param;
    ESP_LOGI(TAG, "Started communication task for port %d", client_port);

    int comm_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    struct sockaddr_in comm_addr;
    comm_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    comm_addr.sin_family = AF_INET;
    comm_addr.sin_port = htons(client_port);

    bind(comm_socket, (struct sockaddr *)&comm_addr, sizeof(comm_addr));
    listen(comm_socket, 1);

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_socket = accept(comm_socket, (struct sockaddr *)&client_addr, &addr_len);

    // Ici, on gère la réception et l'envoi des messages du device
    while (1) {
        char buffer[64];
        int len = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (len > 0) {
            buffer[len] = '\0';
            ESP_LOGI(TAG, "Received from device: %s", buffer);

            // Logique pour envoyer les messages aux serveurs et spies
        }
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    start_wifi_ap();
    start_server();
}
