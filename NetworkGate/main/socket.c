#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "string.h"
#include "esp_log.h"

#define PORT 4000
static const char *TAG_SOCKET = "tcp_server";

// Fonction pour gérer les connexions des clients
void handle_client(int client_socket) {
    char buffer[128];
    int len;
    ESP_LOGI(TAG_SOCKET, "Client connecté.");

    while ((len = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[len] = '\0';  // Assurez-vous que le message est nul-terminé
        ESP_LOGI(TAG_SOCKET, "Message reçu: %s", buffer);
    }

    if (len == 0) {
        ESP_LOGI(TAG_SOCKET, "Client déconnecté.");
    } else {
        ESP_LOGE(TAG_SOCKET, "Erreur de réception.");
    }

    close(client_socket);
}

// Fonction principale du serveur
void tcp_server_task(void *pvParameters) {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        ESP_LOGE(TAG_SOCKET, "Impossible de créer le socket.");
        vTaskDelete(NULL);
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG_SOCKET, "Erreur lors du bind.");
        close(server_socket);
        vTaskDelete(NULL);
        return;
    }

    if (listen(server_socket, 4) < 0) {
        ESP_LOGE(TAG_SOCKET, "Erreur lors de l'écoute.");
        close(server_socket);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG_SOCKET, "Serveur TCP démarré sur le port %d", PORT);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket >= 0) {
            xTaskCreate(handle_client, "handle_client", 4096, (void *)client_socket, 5, NULL);
        } else {
            ESP_LOGE(TAG_SOCKET, "Erreur lors de l'acceptation.");
        }
    }

    close(server_socket);
    vTaskDelete(NULL);
}