#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#define NUM_BUTTONS 9

// Liste des pins correspondant à chaque bouton (ajuste selon tes connexions)
int button_pins[NUM_BUTTONS] = {19, 18, 5, 17, 16, 4, 0, 2, 23};

// Tag pour le logging
static const char *TAG = "BUTTONS";


// Structure pour stocker les événements des boutons
typedef struct {
    int pin;
    int state;
} button_event_t;

// File de messages pour les événements de boutons
static QueueHandle_t button_queue = NULL;


// Fonction de callback pour les interruptions des boutons
void IRAM_ATTR button_isr_handler(void* arg) {
    int pin = (int)arg;
    // Lire l'état du pin (0 = appuyé, 1 = relâché)
    int state = gpio_get_level(pin);
    
    // Créer un événement et l'envoyer dans la file
    button_event_t evt = {
        .pin = pin,
        .state = state
    };
    xQueueSendFromISR(button_queue, &evt, NULL);
}

typedef struct {
    int64_t pushed;
    int64_t released;
} last_events_t;
last_events_t last_events[24];


// Tâche pour gérer les événements de boutons
void button_task(void* arg) {
    button_event_t evt;
    while (1) {
        // Attendre un événement de bouton dans la file
        if (xQueueReceive(button_queue, &evt, portMAX_DELAY)) {
            int btn_state = gpio_get_level(evt.pin);
            
            // Nombre de milisecondes depuis le boot
            int64_t now = esp_timer_get_time() / 1000;
            if (btn_state == 0) {
                last_events[evt.pin].pushed = now;
                last_events[evt.pin].released = 0;
            } else {
                last_events[evt.pin].pushed = 0;
                last_events[evt.pin].released = now;
            }
        }
        
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

// Tache d'enregistrement des événements de boutons
void set_messages(void * params)
{
    const int64_t delta = 30;

    while (1) {
        int64_t now = esp_timer_get_time() / 1000;

        for (int i = 0; i < 24; i++) {
            if (last_events[i].pushed != 0 && (now - last_events[i].pushed) > delta) {
                ESP_LOGW(TAG, "Button %d pushed\n", i);
                last_events[i].pushed = 0;
            }
            if (last_events[i].released != 0 && now - (last_events[i].released) > delta) {
                ESP_LOGW(TAG, "Button %d released\n", i);
                last_events[i].released = 0;
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}


void init_buttons(void) {
    button_queue = xQueueCreate(10, sizeof(button_event_t));
    gpio_install_isr_service(0); // Installer le service ISR (peut être appelé une seule fois)

    // Configuration des pins pour les boutons
    for (int i = 0; i < NUM_BUTTONS; i++) {
        gpio_config_t io_conf = {
            .intr_type = GPIO_INTR_ANYEDGE,    // Déclencher une interruption sur les fronts montants et descendants
            .mode = GPIO_MODE_INPUT,           // Configurer le pin en mode entrée
            .pin_bit_mask = (1ULL << button_pins[i]), // Masque de bit pour sélectionner le pin
            .pull_up_en = GPIO_PULLUP_ENABLE,  // Activer le pull-up interne
            .pull_down_en = GPIO_PULLDOWN_DISABLE // Désactiver le pull-down interne
        };
        gpio_config(&io_conf);
        
        // Attacher l'interruption à la fonction de callback
        gpio_isr_handler_add(button_pins[i], button_isr_handler, (void*) button_pins[i]);
    }

    // Démarrer la tâche pour gérer les événements de boutons
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
    xTaskCreate(set_messages, "set_messages", 2048, NULL, 10, NULL);
    
    ESP_LOGI(TAG, "Button handler initialized.");
}
