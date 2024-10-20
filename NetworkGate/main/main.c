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

static TaskHandle_t * registered_tasks[16] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
static TaskHandle_t * server;
static TaskHandle_t * octopus;
static TaskHandle_t * panels;

# define LB_SIZE 1024
static char letter_boxes[16][LB_SIZE];
static int lb_start[16];
static int lb_free[16];

void init_memory()
{
    // Init the pointers
    server = registered_tasks[0];
    octopus = registered_tasks[1];
    panels = registered_tasks[2];

    for (int i = 0; i < 16; i++)
    {
        lb_start[i] = 0;
        lb_free[i] = 0;
    }
}


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
            .max_connection = 3,
            .password = "tcafetra",
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP started. SSID: %s", WIFI_SSID);
}

static int next_port = PORT + 1;

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
            int client_port = next_port++;  // Ex: à ajuster selon les connexions
            char response[32];
            snprintf(response, sizeof(response), "port %d", client_port);

            send(client_socket, response, strlen(response), 0);
            ESP_LOGI(TAG, "Assigned port %d to device %s", client_port, name);

            // Fermer la socket après l'envoi du port
            shutdown(client_socket, 0);
            close(client_socket);

            // Enregistrer la tâche pour gérer les communications avec ce device
            TaskHandle_t * task = NULL;
            if (strcmp(type, "server") == 0) {
                task = server;
            } else if (strcmp(type, "octopus") == 0) {
                task = octopus;
            } else if (strcmp(type, "panel") == 0) {
                int panel_id = atoi(name + 5);
                task = &(panels[panel_id]);
            } else {
                ESP_LOGE(TAG, "Unknown device type: %s", type);
                return;
            }

            // Delete previous task if needed
            if (task != NULL)
            {
                vTaskDelete(*task);
                task = NULL;
                ESP_LOGI(TAG, "Previous task deleted");
            }

            // Lancer une tâche pour gérer les communications avec ce device
            xTaskCreate(handle_device_communication, "device_task", 4096, (void *)client_port, 5, task);
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

        if (client_socket == -1) {
            ESP_LOGE(TAG, "Error accepting client: %s", strerror(errno));
            continue;
        }

        ESP_LOGI(TAG, "New client connected");

        // Gérer le message d'enregistrement
        handle_register(client_socket, client_addr);
    }
}

void handle_message(char * buffer, int buff_len);

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
    shutdown(comm_socket, 0);
    close(comm_socket);

    if (client_socket == -1) {
        ESP_LOGE(TAG, "Error accepting client: %s", strerror(errno));
        vTaskDelete(NULL);
    }

    // Ici, on gère la réception et l'envoi des messages du device
    while (1) {
        // Réception des messages du device
        char buffer[64];
        int len = recv(client_socket, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
        if (len > 0) {
            buffer[len] = '\0';
            ESP_LOGI(TAG, "Received from device: %s", buffer);
            handle_message(buffer, len);

            // Logique pour envoyer les messages aux serveurs et spies
        } else if (len == -1 && errno != EAGAIN) 
        {
            ESP_LOGE(TAG, "Error receiving from device: %s. Closing connection", strerror(errno));
            shutdown(client_socket, 0);
            close(client_socket);
            vTaskDelete(NULL);
        }

        // Envoi des messages au device
        
        taskYIELD();
    }
}

void handle_message(char * buffer, int buff_len)
{
    // Extract the destination from the first word (until the first space)
    char dest[16];
    sscanf(buffer, "%15s", dest);

    // Select the correct letter box identifier
    int lb_idx = -1;
    if (strncmp(dest, "panel", 5) == 0) {
        int panel_id = atoi(dest + 5);
        lb_idx = 2 + panel_id;
    } else if (strncmp(dest, "server", 6) == 0)
        lb_idx = 0;
    else if (strncmp(dest, "octopus", 7) == 0)
        lb_idx = 1;

    // If the destination is not valid, return
    if (lb_idx == -1)
    {
        ESP_LOGE(TAG, "Invalid message destination: %s", dest);
        return;
    }

    // Get the letter box
    char * letter_box = letter_boxes[lb_idx];
    int * start = lb_start + lb_idx;
    int * free = lb_free + lb_idx;

    // Copy the message in the letter box
    for (int i = 0; i < buff_len; i++)
    {
        if ((*free + 1) % LB_SIZE == *start)
        {
            ESP_LOGE(TAG, "Letter box %d is full", lb_idx);
            return;
        }
        letter_box[(*start + *free) % LB_SIZE] = buffer[i];
        *free = (*free + 1) % LB_SIZE;
    }

    ESP_LOGI(TAG, "Remaining space in letter box %d: %d", lb_idx, (LB_SIZE + *start - *free) % LB_SIZE);
}

void app_main(void) {
    init_memory();

    // Init the  esp functionalities
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Start the wifi ap
    start_wifi_ap();
    // Start the server
    start_server();
}
