#include <esp_log.h>
#include <lwip/sockets.h>



#include "sockets.h"
#include "wifi.h"

static const char *TAG = "sockets";

#define SERVER_IP       "192.168.4.1"
#define SERVER_PORT     8080


typedef struct params_s {
    int permanent_port;
    int perm_sock;
    TaskHandle_t recv_task;
    TaskHandle_t send_task;
} params_t;
static params_t global_params;


// Déclaration des fonctions
void permanent_connection(void * params);
void receive_messages(void * params);
void send_messages(void * params);



void first_connection(void * params)
{
    wait_for_wifi();

    // Récupérer l'adresse MAC de l'ESP32
    char mac_str[18];
    get_mac(mac_str);

    // Variables liées au socket
    int addr_family;
    int ip_protocol;
    struct sockaddr_in dest_addr;

    // Configuration de l'adresse du serveur
    dest_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(SERVER_PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;

    int sock = -1;
    while (sock < 0)
    {
        sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            shutdown(sock, 0);
            close(sock);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
    

    ESP_LOGI(TAG, "Socket created, connecting to %s:%d", SERVER_IP, SERVER_PORT);

    // Essaye de se connecter au serveur en boucle jusqu'à ce que la connexion soit établie
    int err = 1;
    while (err != 0) {
        err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            shutdown(sock, 0);
            close(sock);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }

    ESP_LOGI(TAG, "Successfully connected to the server");

    // Essaye d'envoyer la commande de connexion (5 fois maximum)
    for (int i=0 ; i<5 ; i++) {
        // Envoi de la commande de connexion
        char message[64];
        snprintf(message, sizeof(message), "register panel0 panel %s", mac_str);
        int len = send(sock, message, strlen(message), 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            if (i == 4) {
                shutdown(sock, 0);
                close(sock);
                // Redémarrer la tache au début
                xTaskCreate(first_connection, "first_connection", 4096, NULL, 5, NULL);
                vTaskDelete(NULL);
            }
        } 
        else {
            ESP_LOGI(TAG, "Message d'enregistrement: %s", message);
            break;
        }
    }

    // Réception de l'identifiant de port
    char rx_buffer[64];
    int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
    if (len < 0) {
        ESP_LOGE(TAG, "recv failed: errno %d", errno);
    } else {
        rx_buffer[len] = '\0';
        int new_port = atoi(rx_buffer+5);
        global_params.permanent_port = new_port;
        ESP_LOGI(TAG, "Received port: %d", new_port);

        // Fermer la première connexion
        shutdown(sock, 0);
        close(sock);

        // Délayer la création de la connexion dédiée
        vTaskDelay(500 / portTICK_PERIOD_MS);

        // Creation d'une tache pour la connexion dédiée
        xTaskCreate(permanent_connection, "permanent_connection", 4096, (void *)&global_params, 5, NULL);    
        vTaskDelete(NULL);
    }
}


void permanent_connection(void * params)
{
    params_t *p = (params_t *)params;
   // Variables liées au socket
    int addr_family;
    int ip_protocol;
    struct sockaddr_in dest_addr;

    // Configuration de l'adresse du serveur
    dest_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(p->permanent_port);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;

    int sock = -1;
    while (sock < 0)
    {
        sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            shutdown(sock, 0);
            close(sock);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
    

    ESP_LOGI(TAG, "Socket created, connecting to %s:%d", SERVER_IP, p->permanent_port);

    // Essaye de se connecter au serveur en boucle jusqu'à ce que la connexion soit établie
    for (int i=0 ; i<5 ; i++) {
        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            if (i == 4) {
                shutdown(sock, 0);
                close(sock);
                // Redémarrer la tache au début
                xTaskCreate(permanent_connection, "permanent_connection", 4096, (void *)p, 5, NULL);
                vTaskDelete(NULL);
            }
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
    ESP_LOGI(TAG, "Successfully connected to the server");

    p->perm_sock = sock;

    // Creation de la tache de réception
    xTaskCreate(receive_messages, "receive_messages", 4096, (void *)p, 5, &p->recv_task);
    // Creation de la tache d'envoi
    xTaskCreate(send_messages, "send_messages", 4096, (void *)p, 5, &p->send_task);

    // Suppression de la tache actuelle
    vTaskDelete(NULL);
}


void receive_messages(void * params)
{
    while (1) {
        ESP_LOGI(TAG, "Réception de messages");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        taskYIELD();
    }

    // params_t *p = (params_t *)params;
    // char rx_buffer[64];
    // int len;
    // while (1) {
    //     len = recv(p->perm_sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
    //     if (len < 0) {
    //         ESP_LOGE(TAG, "recv failed: errno %d", errno);
    //         break;
    //     } else if (len == 0) {
    //         ESP_LOGI(TAG, "Connection closed");
    //         break;
    //     } else {
    //         rx_buffer[len] = '\0';
    //         ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);
    //     }
    // }
    // vTaskDelete(NULL);
}


void send_messages(void * params)
{
    while (1) {
        ESP_LOGI(TAG, "Envoi de messages");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        taskYIELD();
    }

    // params_t *p = (params_t *)params;
    // char message[64];
    // int len;
    // while (1) {
    //     // Envoi de la commande de connexion
    //     snprintf(message, sizeof(message), "register octopus octopus %s", mac_str);
    //     len = send(p->perm_sock, message, strlen(message), 0);
    //     if (len < 0) {
    //         ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
    //         vTaskDelay(1000 / portTICK_PERIOD_MS);
    //     } 
    //     else {
    //         ESP_LOGI(TAG, "Message d'enregistrement: %s", message);
    //     }
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
    // vTaskDelete(NULL);
}
    
