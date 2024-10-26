#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#ifndef MSG_HANDLER_H
#define MSG_HANDLER_H

typedef struct msg_param_s {
    int socket;
    TaskHandle_t recv_task;
    TaskHandle_t send_task;
    char name[16];
    char type[16];
} msg_param_t;

void receiver(void * params);
void sender(void * params);

#endif // MSG_HANDLER_H
