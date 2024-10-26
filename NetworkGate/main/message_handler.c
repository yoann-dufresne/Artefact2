#include "lwip/sockets.h"
#include "esp_log.h"
#include <string.h>

#include "message_handler.h"


# define LB_SIZE 1024
static char letter_boxes[16][LB_SIZE];
static int lb_start[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int lb_free[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


void receive_message(char * buffer, int len);
void parse_message(char * buffer, int len); 

void receiver(void * params) {
    static const char *TAG = "receiver";
    msg_param_t * msg_param = (msg_param_t *)params;
    int socket = msg_param->socket;

    char buffer[128];
    int len = 0;
    while (1) {
        len = recv(socket, buffer, sizeof(buffer) - 1, 0);
        if (len > 0) {
            buffer[len] = '\0';
            ESP_LOGI(TAG, "Received: %s\n", buffer);

            receive_message(buffer, len);
        } else if (len == 0) {
            ESP_LOGI(TAG, "Connection closed\n");
            break;
        } else {
            if (errno == ENOTCONN)
                ESP_LOGI(TAG, "Connection closed\n");
            else
                ESP_LOGW(TAG, "recv failed: errno %d\n", errno);
            break;
        }
    }

    ESP_LOGI(TAG, "Déconnexion du client\n");
    // Close the socket
    shutdown(socket, 0);
    close(socket);
    msg_param->socket = -1;
    ESP_LOGI(TAG, "Socket closed");

    // Mets en pause la tâche d'envoi
    if (msg_param->send_task != NULL) {
        ESP_LOGI(TAG, "Deleting send task");
        vTaskDelete(msg_param->send_task);
        msg_param->send_task = NULL;
    }

    // Supprime la tâche de réception
    ESP_LOGI(TAG, "Deleting recv task");
    msg_param->recv_task = NULL;
    vTaskDelete(NULL);
}

#define BUFF_SIZE 1024
static char msg_buffer[BUFF_SIZE];
int first_free = 0;

void receive_message(char * buffer, int len) {
    for (int i = 0; i < len; i++) {
        if (buffer[i] != '\n') {
            msg_buffer[first_free++] = buffer[i];
        }
        else {
            msg_buffer[first_free] = '\0';
            parse_message(msg_buffer, first_free);
            first_free = 0;
        }
    }
}
    

void parse_message(char * buffer, int len) {
    static const char *TAG = "parse_message";
    
    // Extrait le premier mot du message pour connaitre le destinataire
    char dest[16];
    sscanf(buffer, "%16s", dest);

    // Détermine dans quelle boite aux lettres écrire le message
    int lb_idx = -1;
    if (strncmp(dest, "server", 6) == 0) {
        lb_idx = 0;
    } else if (strncmp(dest, "octopus", 7) == 0) {
        lb_idx = 1;
    } else if (strncmp(dest, "panel", 5) == 0) {
        int panel_id = atoi(dest + 5);
        lb_idx = 2 + panel_id;
    }

    // Destinataire inconnu
    if (lb_idx == -1) {
        ESP_LOGW(TAG, "Destinataire inconnu: %s", dest);
        return;
    }

    int msg_start = strlen(dest) + 1;

    // Ecrit le message dans la boite aux lettres
    for (int i = msg_start; i < len; i++) {
        if (((lb_free[lb_idx] + 1) % LB_SIZE) == lb_start[lb_idx]) {
            ESP_LOGW(TAG, "Boite aux lettres pleine pour %s", dest);
            return;
        }
        letter_boxes[lb_idx][lb_free[lb_idx]] = buffer[i];
        lb_free[lb_idx] = (lb_free[lb_idx] + 1) % LB_SIZE;
    }
}


void sender(void * params) {
    static const char *TAG = "sender";
    msg_param_t * msg_param = (msg_param_t *)params;
    int socket = msg_param->socket;

    int lb_idx = -1;
    if (strncmp(msg_param->type, "server", 6) == 0) {
        lb_idx = 0;
    } else if (strncmp(msg_param->type, "octopus", 7) == 0) {
        lb_idx = 1;
    } else if (strncmp(msg_param->type, "panel", 5) == 0) {
        int panel_id = atoi(msg_param->type + 5);
        lb_idx = 2 + panel_id;
    }

    while (1) {
        // Vérifie si un message est disponible
        if (lb_start[lb_idx] < lb_free[lb_idx]) {
            // Envoie le message
            int msg_len = lb_free[lb_idx] - lb_start[lb_idx];
            int len = send(socket, letter_boxes[lb_idx] + lb_start[lb_idx], msg_len, 0);
            if (len < 0) {
                ESP_LOGW(TAG, "send failed: errno %d", errno);
                break;
            }
        } else if (lb_start[lb_idx] > lb_free[lb_idx]) {
            // Envoie le message
            int msg_len = LB_SIZE - lb_start[lb_idx];
            int len = send(socket, letter_boxes[lb_idx] + lb_start[lb_idx], msg_len, 0);
            if (len < 0) {
                ESP_LOGW(TAG, "send failed: errno %d", errno);
                break;
            }

            // Envoie le reste du message
            len = send(socket, letter_boxes[lb_idx], lb_free[lb_idx], 0);
            if (len < 0) {
                ESP_LOGW(TAG, "send failed: errno %d", errno);
                break;
            }
        }
        
        lb_start[lb_idx] = lb_free[lb_idx];

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "Déconnexion du client\n");
    // Close the socket
    shutdown(socket, 0);
    close(socket);
    msg_param->socket = -1;
    ESP_LOGI(TAG, "Socket closed");

    // Mets en pause la tâche d'envoi
    if (msg_param->recv_task != NULL) {
        ESP_LOGI(TAG, "Deleting recv task");
        vTaskDelete(msg_param->recv_task);
        msg_param->recv_task = NULL;
    }

    // Supprime la tâche de réception
    ESP_LOGI(TAG, "Deleting send task");
    msg_param->send_task = NULL;
    vTaskDelete(NULL);
}