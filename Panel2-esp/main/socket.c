#include "socket.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_mac.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <string.h>

static const char *TAG = "socket client";

#define PORT                        4242
#define KEEPALIVE_IDLE              CONFIG_KEEPALIVE_IDLE
#define KEEPALIVE_INTERVAL          CONFIG_KEEPALIVE_INTERVAL
#define KEEPALIVE_COUNT             CONFIG_KEEPALIVE_COUNT

static uint8_t mailbox[256];

void tcp_client_task(void *pvParameters)
{
    memset(mailbox, 0, 256);

    char host_ip[] = "192.168.1.76";
    int addr_family = 0;
    int ip_protocol = 0;

    while (1)
    {
        // Config socket structure
        struct sockaddr_in dest_addr;
        inet_pton(AF_INET, host_ip, &dest_addr.sin_addr);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;

        // Socket creation
        int sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            shutdown(sock, 0);
            close(sock);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, PORT);

        // Set options
        int keepAlive = 1;
        int nodelay = 1;

        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(int));

        // Socket connection
        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            shutdown(sock, 0);
            close(sock);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        ESP_LOGI(TAG, "Successfully connected");

        while (1) {
            // Check for outgoing messages
            if (mailbox[0] > 0) {
                err = send(sock, mailbox, mailbox[0] + 1, 0);
                mailbox[0] = 0;
                if (err < 0) {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    break;
                }
            }

            // Check for incoming messages
            char rx_buffer[256];
            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            if (len < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            } else if (len == 0) {
                ESP_LOGI(TAG, "Connection closed");
                break;
            } else {
                rx_buffer[len] = 0; // Null-terminate the received data
                ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);

                // Check if the message starts with "ping"
                if (strncmp(rx_buffer, "ping", 4) == 0) {
                    uint64_t time_since_boot = esp_timer_get_time() / 1000; // Get time in milliseconds
                    char pong_msg[64];
                    snprintf(pong_msg, sizeof(pong_msg), "pong %llu ms", time_since_boot);
                    err = send(sock, pong_msg, strlen(pong_msg), 0);
                    if (err < 0) {
                        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                        break;
                    }
                }
            }

            vTaskDelay(pdMS_TO_TICKS(10));
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
}

void sock_send(uint8_t *msg, uint8_t size)
{
    memcpy(mailbox + 1, msg, size);
    mailbox[0] = size;
}

void sock_init()
{
    xTaskCreate(tcp_client_task, "tcp_server", 4096, NULL, 5, NULL);
}
