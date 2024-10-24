#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
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
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#define WIFI_SSID "artefact"
#define WIFI_PASS "tcafetra"
#define PORT 8080
static const char *TAG = "octopus";

#include "led_strip.h"
#define NUM_STRIPS 8
#define NUM_LEDS_PER_STRIP 64

static uint8_t led_strip_pins[NUM_STRIPS] = {15, 2, 18, 19, 32, 25, 14, 12};
static uint8_t led_data[NUM_STRIPS][NUM_LEDS_PER_STRIP * 3];

static led_strip_handle_t led_strip[NUM_STRIPS];

void set_led_color(uint8_t *data, uint8_t r, uint8_t g, uint8_t b);

void init_led_strips() {
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = 15, // The GPIO that connected to the LED strip's data line
        .max_leds = NUM_LEDS_PER_STRIP, // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812, // LED strip model
        .flags.invert_out = false, // whether to invert the output signal (useful when your hardware has a level inverter)
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // different clock source can lead to different power consumption
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false, // whether to enable the DMA feature
    };

    for (int i = 0; i < NUM_STRIPS; i++) {
        // Setup the phisical device
        strip_config.strip_gpio_num = led_strip_pins[i];
        ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &(led_strip[i])));

        // Setup the data buffer
        for (int j = 0; j < NUM_LEDS_PER_STRIP; j++) {
            set_led_color(&led_data[i][j * 3], 0, 0, 0);
        }
    }
    
    ESP_LOGI(TAG, "LED strips initialized.");
}

void set_led_color(uint8_t *data, uint8_t r, uint8_t g, uint8_t b) {
    data[0] = g; // Le protocole des LEDs Neopixel utilise l'ordre GRB
    data[1] = r;
    data[2] = b;
}

void set_led_state(int strip_num, int led_num, uint8_t r, uint8_t g, uint8_t b) {
    if (strip_num >= 0 && strip_num < NUM_STRIPS && led_num >= 0 && led_num < NUM_LEDS_PER_STRIP) {
        set_led_color(&led_data[strip_num][led_num * 3], r, g, b);
    }
}

size_t encode_led_data(uint8_t* led_data, rmt_symbol_word_t* symbols, size_t led_count) {
    size_t symbol_idx = 0;
    for (size_t i = 0; i < led_count; i++) {
        for (int bit = 7; bit >= 0; bit--) {
            symbols[symbol_idx].level0 = 1;
            symbols[symbol_idx].duration0 = (led_data[i] & (1 << bit)) ? 8 : 4;
            symbols[symbol_idx].level1 = 0;
            symbols[symbol_idx].duration1 = (led_data[i] & (1 << bit)) ? 4 : 8;
            symbol_idx++;
        }
    }
    return symbol_idx;
}

void refresh_led_strip(int strip_num) {
    ESP_LOGI(TAG, "Refreshing LED strip %d", strip_num);
    // Update the leds with the new data
    for (int led_idx=0 ; led_idx<NUM_LEDS_PER_STRIP ; led_idx++) {
        led_strip_set_pixel(led_strip[strip_num], led_idx, led_data[strip_num][led_idx*3], led_data[strip_num][led_idx*3+1], led_data[strip_num][led_idx*3+2]);
    }
    // Refrech the led strip physically
    led_strip_refresh(led_strip[strip_num]);
}

void update_led_strip_with_array(int strip_num, char color_array[32]) {
    ESP_LOGI(TAG, "update leds %d: %32s", strip_num, color_array);

    if (strip_num >= 0 && strip_num < NUM_STRIPS) {
        for (int i = 0; i < 32; i++) {
            uint8_t r = 0, g = 0, b = 0;
            switch (color_array[i]) {
                case 'v': g = 20; break;
                case 'r': r = 20; break;
                case 'l': b = 20; break;
                case 'n': r = g = b = 0; break;
                case 'b': r = g = b = 20; break;
                default: break;
            }
            set_led_state(strip_num, i * 2, r, g, b);
            set_led_state(strip_num, i * 2 + 1, r, g, b);
        }
        refresh_led_strip(strip_num);
    }
}


// ------------------- WIFI -------------------

static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

// Gestionnaire des événements Wi-Fi
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Réessai de connexion au Wi-Fi...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Adresse IP acquise: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void) {
    // Initialisation du Wi-Fi en mode station
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Enregistrement du gestionnaire d'événements pour la gestion du Wi-Fi et de l'IP
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    // Configuration du Wi-Fi
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta terminé.");
    ESP_LOGI(TAG, "Connexion au réseau Wi-Fi : %s...", WIFI_SSID);
}


// ------------------- SOCKET -------------------

#define SERVER_IP       "192.168.4.1"
#define SERVER_PORT     8080

void parse_message(char* message, int message_len);
void receive_messages(int socket);

void socket_task(void *pvParameters) {
    int addr_family;
    int ip_protocol;
    struct sockaddr_in dest_addr;

    while (1) {
        // Attendre que le Wi-Fi soit connecté
        xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

        // Récupérer l'adresse MAC de l'ESP32
        uint8_t mac[6];
        esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
        char mac_str[18];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        // Configuration de l'adresse du serveur
        dest_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(SERVER_PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;

        int sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "Socket created, connecting to %s:%d", SERVER_IP, SERVER_PORT);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            close(sock);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "Successfully connected to the server");

        // Envoi de la commande de registre
        char message[64];
        snprintf(message, sizeof(message), "register octopus octopus %s", mac_str);
        int len = send(sock, message, strlen(message), 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            close(sock);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "Sent message: %s", message);

        // Réception de l'identifiant de port
        char rx_buffer[64];
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            ESP_LOGE(TAG, "recv failed: errno %d", errno);
        } else {
            rx_buffer[len] = '\0';
            int new_port = atoi(rx_buffer+5);
            ESP_LOGI(TAG, "Received port: %d", new_port);

            // Fermer la première connexion
            close(sock);

            vTaskDelay(20 / portTICK_PERIOD_MS);

            // Ouvrir une nouvelle connexion avec le port reçu
            dest_addr.sin_port = htons(new_port);
            sock = socket(addr_family, SOCK_STREAM, ip_protocol);
            if (sock < 0) {
                ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                continue;
            }

            err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            if (err != 0) {
                ESP_LOGE(TAG, "Socket unable to connect to new port: errno %d", errno);
                close(sock);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                continue;
            }

            ESP_LOGI(TAG, "Successfully connected to %s:%d", SERVER_IP, new_port);
        }

        // Traiter les communications avec le serveur ici...
        receive_messages(sock);

        shutdown(sock, 0);
        close(sock);

        vTaskDelay(3000 / portTICK_PERIOD_MS); // Attendre avant de retenter la connexion
    }
}

/**
 * Fonction pour recevoir les messages qui peuvent être fractonnés en morceaux.
 * Un message termine par le charactère \n.
 * Si un message incomplet est reçu, il est stocké dans un buffer en attendant de recevoir la suite.
 * Les message est envoyé pour parsing lorsqu'un message complet est reçu.
 */
void receive_messages(int socket)
{
    char message_buffer[256];
    char rx_buffer[129];
    int message_buffer_idx = 0;
    int len;
    while (1) {
        len = recv(socket, rx_buffer, sizeof(rx_buffer)-1, 0);
        if (len < 0) {
            ESP_LOGE(TAG, "recv failed: errno %d", errno);
            break;
        } else if (len == 0) {
            ESP_LOGI(TAG, "Connection closed");
            break;
        } else {
            rx_buffer[len] = '\0';
            ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);

            // Traiter le message reçu
            for (int i = 0; i < len; i++) {
                if (rx_buffer[i] == '\n') {
                    message_buffer[message_buffer_idx] = '\0';
                    ESP_LOGI(TAG, "Received message: %s", message_buffer);
                    parse_message(message_buffer, message_buffer_idx);
                    message_buffer_idx = 0;
                } else {
                    message_buffer[message_buffer_idx] = rx_buffer[i];
                    message_buffer_idx++;
                }
            }
        }
    }
}


void parse_message(char* message, int message_len) {
    if (message_len < 34) {
        ESP_LOGE(TAG, "Message trop court: %s", message);
        return;
    }

    // Vérification que le premier char est l'index du ruban de leds
    if (message[0] < '0' || message[0] > '7') {
        ESP_LOGE(TAG, "Mauvais destinataire dans les message: %s", message);
        return;
    }
    int idx = message[0] - '0';

    update_led_strip_with_array(idx, message+2);
}


// ------------------- MAIN -------------------

void app_main(void) {
    s_wifi_event_group = xEventGroupCreate();

    // Initialisation de NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    // Initialisation de l'interface réseau
    ESP_ERROR_CHECK(esp_netif_init());
    // Initialisation de l'évènement de connexion au réseau
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // Initialisation de la connexion au réseau
    wifi_init_sta();

    // Initialisation des rubans de leds
    init_led_strips();

    // Initialisation du socket
    xTaskCreate(socket_task, "socket_task", 4096, NULL, 5, NULL);

    while (1)
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
