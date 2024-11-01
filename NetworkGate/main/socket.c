#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_log.h"

#include "message_handler.h"

#define SERVER_PORT     8080


static const char *TAG = "socket_server";

static int listen_soket;

typedef struct client_s {
    int in_use;
    char mac[18];
    msg_param_t msg_param;
} client_t;
static client_t clients[12];

void registration(void * params);



void clean_task()
{
    if (listen_soket != -1)
    {
        shutdown(listen_soket, 0);
        close(listen_soket);
    }

    vTaskDelete(NULL);
}

void tcp_server_task(void *pvParameters) {
    
    char addr_str[128];
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;
    struct sockaddr_in dest_addr;

    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(SERVER_PORT);

    // Création du socket
    listen_soket = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_soket < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        clean_task();
    }

    // Liaison du socket à l'adresse et au port
    int err = bind(listen_soket, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        clean_task();
    }
    ESP_LOGI(TAG, "Socket bound, port %d", SERVER_PORT);

    // Écoute des connexions entrantes
    err = listen(listen_soket, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        clean_task();
    }

    ESP_LOGI(TAG, "Socket listening...");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_sock = accept(listen_soket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Convertir l'adresse IP pour l'affichage
        inet_ntoa_r(((struct sockaddr_in *)&client_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
        ESP_LOGI(TAG, "Socket accepted connection from %s", addr_str);

        // Recherche d'un client libre
        int i = 0;
        for (i = 0; i < 12; i++)
        {
            if (clients[i].in_use == 0)
            {
                clients[i].in_use = 1;
                clients[i].msg_param.socket = client_sock;
                break;
            }
        }

        // Démarre les taches de communication avec le device
        xTaskCreate(registration, "receiver", 4096, (void *)&(clients[i]), 5, NULL);
    }

   clean_task();
}


void registration(void * params) {
    static const char *TAG = "registration_task";
    client_t * client = (client_t *)params;
    int socket = client->msg_param.socket;

    char buffer[128];
    int len = 0;
    while (1) {
        // TODO: ajouter un timeout pour killer la tâche si pas de message reçu
        len = recv(socket, buffer, sizeof(buffer) - 1, 0);
        if (len > 0) {
            buffer[len] = '\0';
            ESP_LOGI(TAG, "Received: %s\n", buffer);

            // try to extract values from the registration message. Should be "register name type mac"
            char name[16], type[16], mac[18];
            if (sscanf(buffer, "register %16s %16s %18s", name, type, mac) != 3) {
                ESP_LOGE(TAG, "Registration message not valid");
                continue;
            }

            // Détruit les précédents processus de communication avec une adresse MAC identique
            for (int i = 0; i < 12; i++)
            {
                if (strcmp(clients[i].mac, mac) == 0)
                {
                    ESP_LOGI(TAG, "Client with MAC %s was already registered. Cleaning up...", mac);
                    // Close the socket
                    if (clients[i].msg_param.socket != -1) {
                        ESP_LOGI(TAG, "Closing socket %d", clients[i].msg_param.socket);
                        shutdown(clients[i].msg_param.socket, 0);
                        close(clients[i].msg_param.socket);
                        clients[i].msg_param.socket = -1;
                        // Efface l'adresse MAC
                        memset(clients[i].mac, 0, 18);
                        clients[i].in_use = 0;
                        break;
                    }
                }
            }

            // Save the mac address
            strcpy(client->mac, mac);
            strcpy(client->msg_param.name, name);
            strcpy(client->msg_param.type, type);

            // Send the registration confirmation
            char message[64];
            snprintf(message, sizeof(message), "ok");
            len = send(socket, message, strlen(message), 0);
            if (len < 0) {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                break;
            }

            // Start the recv task
            xTaskCreate(receiver, "receiver", 4096, (void *)&(client->msg_param), 5, &(client->msg_param.recv_task));
            // Start the send task
            xTaskCreate(sender, "sender", 4096, (void *)&(client->msg_param), 5, &(client->msg_param.send_task));

            vTaskDelete(NULL);

        } else if (len == 0) {
            ESP_LOGI(TAG, "Connection closed\n");
            break;
        } else {
            ESP_LOGE(TAG, "recv failed: errno %d\n", errno);
            break;
        }
    }

    ESP_LOGI(TAG, "Déconnexion du client\n");
    // Close the socket
    shutdown(socket, 0);
    close(socket);

    // Supprime la tâche de réception
    vTaskDelete(NULL);
}