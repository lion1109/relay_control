#include "build.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <driver/usb_serial_jtag.h>

#include <string.h>
#include <math.h>

#include "relay_control.h"

#pragma message "compiling relay_control"

#define APP_NAME "relay control"
#define TAG "relay_control"

#define RELAY_1 GPIO_NUM_10
#define RELAY_2 GPIO_NUM_1
#define RELAY_3 GPIO_NUM_0

// initialize by board_attributes:
uint16_t board_id = 0; // 16 bit network device address 

int current_state = -1;

int relay_get_state() {
    return current_state;
}

void handle_command() {
    printf("handle_command\n");

}

void fatal_error(const char* error) {
    LOGE(TAG, "fatal error: %s\r\n", error);
    while (1) vTaskDelay(pdMS_TO_TICKS(10000));
}

static void relay_task(void* arg) {
    LOGI(TAG, "relay_control task started");

    uint8_t prev = 0;
    // uint8_t current;

    while (true) {
        /*int c = getchar();
        
        if (c != EOF) {
            LOGI(TAG, "read %d", c);

            if (prev == 0x41 && current == 0x30) {
                handle_command();
            }

            prev = current;
        }'*/

        gpio_set_level(RELAY_1, 0);
        gpio_set_level(RELAY_2, 0);
        gpio_set_level(RELAY_3, 0);

        vTaskDelay(pdMS_TO_TICKS(1000));

        gpio_set_level(RELAY_1, 1);
        gpio_set_level(RELAY_2, 1);
        gpio_set_level(RELAY_3, 1);

        vTaskDelay(pdMS_TO_TICKS(1000));

        gpio_set_level(RELAY_1, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(RELAY_1, 1);

        vTaskDelay(pdMS_TO_TICKS(1000));

        gpio_set_level(RELAY_2, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(RELAY_2, 1);

        vTaskDelay(pdMS_TO_TICKS(1000));

        gpio_set_level(RELAY_3, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(RELAY_3, 1);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    fatal_error("relay_control task end");
}

#ifndef UNIT_TEST
extern "C" void app_main() {
    printf("\nStart FreeRTOS implementation: %s\n", APP_NAME);

    // configure gpio
    gpio_reset_pin(RELAY_1);
    gpio_set_direction(RELAY_1, GPIO_MODE_OUTPUT);

    gpio_reset_pin(RELAY_2);
    gpio_set_direction(RELAY_2, GPIO_MODE_OUTPUT);

    gpio_reset_pin(RELAY_3);
    gpio_set_direction(RELAY_3, GPIO_MODE_OUTPUT);

    // most relais are active low, so switch of by setting high 
    gpio_set_level(RELAY_1, 1);
    gpio_set_level(RELAY_2, 1);
    gpio_set_level(RELAY_3, 1);

    // initialize uart
    usb_serial_jtag_driver_config_t config = {
        .tx_buffer_size = 256,
        .rx_buffer_size = 256,
    };
    usb_serial_jtag_driver_install(&config);

    // create tasks
    xTaskCreatePinnedToCore(
      relay_task,      // Funktion für UWB-Empfang
      "relay_control", // Name Task
      4096,        // Stack
      NULL,        // Parameter
      12,          // Priorität
      NULL,        // Task Handle
      0            // Core 0
    );
  }
#endif