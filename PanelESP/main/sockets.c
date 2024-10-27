#include <esp_log.h>
#include <lwip/sockets.h>



#include "sockets.h"
#include "wifi.h"
#include "leds.h"
#include "panel.h"

static const char *TAG = "sockets";

#define SERVER_IP       "192.168.4.1"
#define SERVER_PORT     8080


typedef struct params_s {
    int sock;
    TaskHandle_t recv_task;
    TaskHandle_t send_task;
} params_t;
static params_t global_params;


static char letter_box[1024];
static int lb_first_free = 0;


// Déclaration des fonctions
void permanent_connection(void * params);
void receive_messages(void * params);
void send_messages(void * params);



void init_connection(void * params)
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
        snprintf(message, sizeof(message), "register panel%d panel %s", PANEL_ID, mac_str);
        int len = send(sock, message, strlen(message), 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            if (i == 4) {
                shutdown(sock, 0);
                close(sock);
                // Redémarrer la tache au début
                xTaskCreate(init_connection, "first_connection", 4096, NULL, 5, NULL);
                vTaskDelete(NULL);
            }
        } 
        else {
            ESP_LOGI(TAG, "Message d'enregistrement: %s", message);
            break;
        }
    }

    global_params.sock = sock;
    // Creation de la tache de réception
    xTaskCreate(receive_messages, "receive_messages", 4096, (void *)&global_params, 5, &(global_params.recv_task));
    // Creation de la tache d'envoi
    xTaskCreate(send_messages, "send_messages", 4096, (void *)&global_params, 5, &(global_params.send_task));

    // Suppression de la tache actuelle
    vTaskDelete(NULL);
}


void process_message(char *message, int len);
void parse_message(char *message, int len);


void receive_messages(void * params)
{
    params_t *p = (params_t *)params;
    char rx_buffer[64];
    int len;
    while (1) {
        len = recv(p->sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            ESP_LOGE(TAG, "recv failed: errno %d", errno);
            break;
        } else if (len == 0) {
            ESP_LOGI(TAG, "Connection closed");
            break;
        } else {
            rx_buffer[len] = '\0';
            ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);
            process_message(rx_buffer, len);
        }
    }
    vTaskDelete(NULL);
}


#define BUFF_SIZE 1024
static char msg_buff[BUFF_SIZE];
static int first_free = 0;

void process_message(char *message, int len)
{
    for (int i=0 ; i<len ; i++) {
        if (message[i] == '\n') {
            msg_buff[first_free] = '\0';
            parse_message(msg_buff, first_free);
            first_free = 0;
        }
        else {
            msg_buff[first_free] = message[i];
            first_free += 1;
        }
    }
}


void parse_message(char *message, int len)
{
    if (len != 9) {
        ESP_LOGE(TAG, "Message invalide: %s", message);
        return;
    }

    // Adapte l'ordre des couleurs pour correspondre à l'ordre des boutons
    char reordered[9];
    for (int i=0 ; i<8 ; i++) {
        reordered[i] = message[logical_to_hardware_button(i)];
    }
    reordered[8] = message[8];

    update_led_strip_with_array(reordered);
}


void send_messages(void * params)
{
    params_t *p = (params_t *)params;
    char message[64];
    int len;
    while (1) {
        if (lb_first_free > 0) {
            // Envoi de la commande de connexion
            len = send(p->sock, letter_box, lb_first_free, 0);
            lb_first_free = 0;
            if (len < 0) {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                vTaskDelay(100 / portTICK_PERIOD_MS);
            } 
            else {
                ESP_LOGD(TAG, "Message envoyé: %s", message);
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}


void register_msg(char * msg, int len)
{
    // Envoi de la commande de connexion
    memcpy(letter_box + lb_first_free, msg, len);
    lb_first_free += len;
    letter_box[lb_first_free] = '\n';
    lb_first_free += 1;
}
    
