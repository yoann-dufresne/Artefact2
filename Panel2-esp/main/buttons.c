#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "buttons.h"
#include "socket.h"


// 19, 18, 5, 17, 16, 4, 0, 2
#define BTN0_GPIO 19
#define BTN1_GPIO 18
#define BTN2_GPIO 5
#define BTN3_GPIO 17
#define BTN4_GPIO 16
#define BTN5_GPIO 4
#define BTN6_GPIO 0
#define BTN7_GPIO 2
#define CENTRAL_BTN 23

#define INPUT_MASK ((1ULL << BTN0_GPIO) | (1ULL << BTN1_GPIO) | (1ULL << BTN2_GPIO) | (1ULL << BTN3_GPIO) | (1ULL << BTN4_GPIO) | (1ULL << BTN5_GPIO) | (1ULL << BTN6_GPIO) | (1ULL << BTN7_GPIO) | (1ULL << CENTRAL_BTN))

static const uint32_t gpio_to_btn[] = {
      6, 127,   7, 127,   5,
      2, 127, 127, 127, 127,
    127, 127, 127, 127, 127,
    127,   4,   3,   1,   0,
    127, 127, 127,   8, 127,
    127, 127, 127, 127, 127,
    127, 127
};


#define ESP_INTR_FLAG_DEFAULT 0
static QueueHandle_t gpio_evt_queue = NULL;

static TickType_t last_ticks[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    uint32_t btn_idx = gpio_to_btn[gpio_num];

    TickType_t current = xTaskGetTickCountFromISR();
    TickType_t delay = current - last_ticks[btn_idx];
    uint32_t ms = pdTICKS_TO_MS(delay);

    if (ms < 100)
    	return;

    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
    last_ticks[btn_idx] = current;
}

static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%"PRIu32"] intr, val: %d\n", io_num, gpio_get_level(io_num));
            uint8_t msg[] = {'B', '9', '0'};
            sock_send(msg, 1);
        }
    }
}




void btns_init()
{
	//zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = INPUT_MASK;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(BTN1_GPIO, gpio_isr_handler, (void*) BTN1_GPIO);
}