#include <esp_log.h>
#include <lwip/sockets.h>


#include "sockets.h"
#include "wifi.h"

static const char *TAG = "sockets";

#define SERVER_IP       "192.168.4.1"
#define SERVER_PORT     8080


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
        snprintf(message, sizeof(message), "register octopus octopus %s", mac_str);
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
        ESP_LOGI(TAG, "Received port: %d", new_port);

        // Fermer la première connexion
        shutdown(sock, 0);
        close(sock);

        // Creation d'une tache pour la connexion dédiée
        xTaskCreate(permanent_connection, "permanent_connection", 4096, NULL, 5, NULL);    
        vTaskDelete(NULL);
    }
}


void permanent_connection(void * params)
{
    while(1) {
        ESP_LOGI(TAG, "Tache de connexion permanente");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        taskYIELD();
    }
}
    

// void parse_message(char* message, int message_len);
// void receive_messages(int socket);

// void socket_task(void *pvParameters) {
//     

//     while (1) {

//         // Traiter les communications avec le serveur ici...
//         receive_messages(sock);

//         shutdown(sock, 0);
//         close(sock);

//         vTaskDelay(3000 / portTICK_PERIOD_MS); // Attendre avant de retenter la connexion
//     }
// }

// /**
//  * Fonction pour recevoir les messages qui peuvent être fractonnés en morceaux.
//  * Un message termine par le charactère \n.
//  * Si un message incomplet est reçu, il est stocké dans un buffer en attendant de recevoir la suite.
//  * Les message est envoyé pour parsing lorsqu'un message complet est reçu.
//  */
// void receive_messages(int socket)
// {
//     char message_buffer[256];
//     char rx_buffer[129];
//     int message_buffer_idx = 0;
//     int len;
//     while (1) {
//         len = recv(socket, rx_buffer, sizeof(rx_buffer)-1, 0);
//         if (len < 0) {
//             ESP_LOGE(TAG, "recv failed: errno %d", errno);
//             break;
//         } else if (len == 0) {
//             ESP_LOGI(TAG, "Connection closed");
//             break;
//         } else {
//             rx_buffer[len] = '\0';
//             ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);

//             // Traiter le message reçu
//             for (int i = 0; i < len; i++) {
//                 if (rx_buffer[i] == '\n') {
//                     message_buffer[message_buffer_idx] = '\0';
//                     ESP_LOGI(TAG, "Received message: %s", message_buffer);
//                     parse_message(message_buffer, message_buffer_idx);
//                     message_buffer_idx = 0;
//                 } else {
//                     message_buffer[message_buffer_idx] = rx_buffer[i];
//                     message_buffer_idx++;
//                 }
//             }
//         }
//     }
// }


// void parse_message(char* message, int message_len) {
//     if (message_len < 34) {
//         ESP_LOGE(TAG, "Message trop court: %s", message);
//         return;
//     }

//     // Vérification que le premier char est l'index du ruban de leds
//     if (message[0] < '0' || message[0] > '7') {
//         ESP_LOGE(TAG, "Mauvais destinataire dans les message: %s", message);
//         return;
//     }
//     int idx = message[0] - '0';

//     update_led_strip_with_array(idx, message+2);
// }
