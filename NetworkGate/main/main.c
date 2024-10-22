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


/**
 * Cherche dans la liste des appreils connectés l'adresse MAC.
 * Retourne 1 si l'adresse MAC a été trouvée, 0 sinon.
 */
int is_disconnected(char* mac);


// -------- Server functions --------

void handle_device_communication(void *param);

static int next_port = PORT + 1;

void handle_register(int client_socket, struct sockaddr_in client_addr) {
    char buffer[64];
    struct timeval timeout;
    timeout.tv_sec = 0;  // Timeout in seconds
    timeout.tv_usec = 10000; // Additional microseconds

    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    int len = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (len > 0) {
        buffer[len] = '\0';
        ESP_LOGI(TAG, "Received registration message: %s", buffer);

        // Analyse du message
        char name[33], type[16], mac[33];
        if (sscanf(buffer, "register %32s %15s %s32", name, type, mac) == 3) {
            ESP_LOGI(TAG, "Device name: %s, Type: %s, mac: %s", name, type, mac);

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
            int lb_idx = -1;
            if (strcmp(type, "server") == 0) {
                task = server;
                lb_idx = 0;
            } else if (strcmp(type, "octopus") == 0) {
                task = octopus;
                lb_idx = 1;
            } else if (strcmp(type, "panel") == 0) {
                int panel_id = atoi(name + 5);
                task = &(panels[panel_id]);
                lb_idx = 2 + panel_id;
            } else {
                ESP_LOGE(TAG, "Unknown device type: %s", type);
                return;
            }

            // Delete previous task if needed
            if (task != NULL)
            {
                vTaskDelete(*task);
                task = NULL;
                // ESP_LOGI(TAG, "Previous task deleted");
            }

            // Lancer une tâche pour gérer les communications avec ce device
            void * params = malloc(2 * sizeof(int) + 18 * sizeof(char));
            ((int*)params)[0] = client_port;
            ((int*)params)[1] = lb_idx;
            char * mac_param = (char *)(((int *)params)+2);
            memcpy(mac_param, mac, 18);

            xTaskCreate(handle_device_communication, "device_task", 4096, (void *)params, 5, task);
        } else {
            ESP_LOGE(TAG, "Invalid registration message");
        }
    } else {
        shutdown(client_socket, 0);
        close(client_socket);
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

        // ESP_LOGI(TAG, "New client connected");

        // Gérer le message d'enregistrement
        handle_register(client_socket, client_addr);
    }
}

// -------- Device communication functions --------

void device_loop(int client_socket, int lb_idx, char * mac_addresse);

void handle_device_communication(void *param) {
    int * tmp_ptr = (int*)param;
    int client_port = tmp_ptr[0];
    int lb_idx = tmp_ptr[1];
    char * mac_address = (char *)(tmp_ptr+2);

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

    device_loop(client_socket, lb_idx , mac_address);
}

void handle_message(char * buffer, int buff_len);
void send_messages(int lb_idx, int socket);
int device_not_connected(int socket);

void device_loop(int client_socket, int lb_idx, char * mac_addresse) {
    // Ici, on gère la réception et l'envoi des messages du device
    
    // Enregistrement du temps de la dernière communication (temps depuis le démarage de l'esp32)
    int64_t last_comm_time = esp_timer_get_time() / 1000;

    while (1) {
        // Réception des messages du device
        char buffer[64];
        int len = recv(client_socket, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
        
        if (len > 0) {
            buffer[len] = '\0';
            ESP_LOGI(TAG, "Received from device: %s", buffer);
            handle_message(buffer, len);
            last_comm_time = esp_timer_get_time() / 1000;
        } else if (len == 0)
        {
            ESP_LOGI(TAG, "Device disconnected. Closing connection");
            shutdown(client_socket, 0);
            close(client_socket);
            vTaskDelete(NULL);
        } 
        else if (len == -1 && errno != EAGAIN) 
        {
            ESP_LOGE(TAG, "Error receiving from device: %s. Closing connection", strerror(errno));
            shutdown(client_socket, 0);
            close(client_socket);
            vTaskDelete(NULL);
        }
        else if (len == -1)
        {
            int64_t delay_since_last_contact = (esp_timer_get_time() / 1000) - last_comm_time;
            
            if (delay_since_last_contact > 1000)
            {
                last_comm_time = esp_timer_get_time() / 1000;
                if (is_disconnected(mac_addresse))
                {
                    ESP_LOGI(TAG, "Device disconnected. Cleaning connection and task");
                    shutdown(client_socket, 0);
                    close(client_socket);
                    vTaskDelete(NULL);
                }
            }
        }

        taskYIELD();

        send_messages(lb_idx, client_socket);
        
        taskYIELD();
    }
}

void send_messages(int lb_idx, int socket)
{
    // Envoi des messages au device
    int start = lb_start[lb_idx];
    int free = lb_free[lb_idx];

    if (free != start)
    {
        // ESP_LOGI(TAG, "before sending msg from letter box %d: [%d ; %d]", lb_idx, lb_start[lb_idx], lb_free[lb_idx]);
        int len_sent = 0;
        if (free > start)
        {
            int len = free - start;
            len_sent = send(socket, letter_boxes[lb_idx] + start, len, MSG_DONTWAIT);
            if (len_sent == -1 && errno != EAGAIN)
                    ESP_LOGE(TAG, "Error sending to device: %s. ", strerror(errno));
            else
                lb_start[lb_idx] = free;
        }
        else
        {
            int len = LB_SIZE - start;
            len_sent = send(socket, letter_boxes[lb_idx] + start, len, MSG_DONTWAIT);
            if (len_sent == -1 && errno != EAGAIN)
                ESP_LOGE(TAG, "Error sending to device: %s. ", strerror(errno));
            else
            {
                lb_start[lb_idx] = 0;
                len = send(socket, letter_boxes[lb_idx], free, MSG_DONTWAIT);
                if (len == -1 && errno != EAGAIN)
                    ESP_LOGE(TAG, "Error sending to device: %s. ", strerror(errno));
                else
                    lb_start[lb_idx] = free;
            }
        }

        // ESP_LOGI(TAG, "after sending msg from letter box %d: [%d ; %d]", lb_idx, lb_start[lb_idx], lb_free[lb_idx]);
    }
}

void add_msg (char * msg, int msg_len);

static char pending_msg[64];
static int first_free_pending = 0;

void handle_message(char * buffer, int buff_len)
{
    // Pre-process the messages to make sure that they are complete before forwarding them
    for (int i=0 ; i<buff_len ; i++)
    {
        if (buffer[i] == '\n') {
            pending_msg[first_free_pending] = '\0';
            add_msg(pending_msg, first_free_pending);
            first_free_pending = 0;
        }
        else {
            pending_msg[first_free_pending++] = buffer[i];
        }
    }
}


void add_msg (char * msg, int msg_len)
{
    // Extract the destination from the first word (until the first space)
    char dest[16];
    sscanf(msg, "%15s", dest);
    int head_len = strlen(dest);
    // ESP_LOGI(TAG, "Destination (%d): %s", head_len, dest);

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
    }
    else {
        // Get the letter box
        char * letter_box = letter_boxes[lb_idx];
        int * start = lb_start + lb_idx;
        int * free = lb_free + lb_idx;

        // Copy the message in the letter box
        for (int i = 0; i < msg_len - head_len - 1; i++)
        {
            if ((*free + 1) % LB_SIZE == *start)
            {
                ESP_LOGE(TAG, "Letter box %d is full", lb_idx);
                return;
            }
            // ESP_LOGI(TAG, "write %c at %d", buffer[i], (*free) % LB_SIZE);
            letter_box[(*free) % LB_SIZE] = msg[i + head_len + 1];
            *free = (*free + 1) % LB_SIZE;
        }
    }

    // ESP_LOGI(TAG, "letter box %d: [%d ; %d]", lb_idx, *start, *free);
    // ESP_LOGI(TAG, "Remaining space in letter box %d: %d", lb_idx, (LB_SIZE + *start - *free) % LB_SIZE);
}

// -------- WIFI functions --------


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

    // Enregistrement du gestionnaire d'événements
    // ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &wifi_event_handler, NULL));


    ESP_LOGI(TAG, "WiFi AP started. SSID: %s", WIFI_SSID);
}

// Convert MAC address to a string format for comparison
void mac_to_str(const uint8_t *mac, char *mac_str) {
    sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

int is_disconnected(char* mac)
{
    wifi_sta_list_t wifi_sta_list;
    esp_err_t err = esp_wifi_ap_get_sta_list(&wifi_sta_list);
    if (err != ESP_OK) {
        ESP_LOGE("NetworkGate", "Failed to get STA list: %s", esp_err_to_name(err));
        return 0; // Suppose que l'appareil n'est pas connecté en cas d'erreur
    }

    char sta_mac[18]; // Format pour une adresse MAC sous forme de chaîne

    for (int i = 0; i < wifi_sta_list.num; i++) {
        mac_to_str(wifi_sta_list.sta[i].mac, sta_mac);
        // ESP_LOGI("NetworkGate", "STA %d MAC: %s", i, sta_mac);
        if (strcmp(mac, sta_mac) == 0) {
            return 0; // L'appareil est connecté
        }
    }

    return 1; // L'appareil n'est pas connecté
}



// -------- Main function --------

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
