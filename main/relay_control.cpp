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

void fatal_error(const char* error) {
    LOGE(TAG, "fatal error: %s\r\n", error);
    while (1) vTaskDelay(pdMS_TO_TICKS(10000));
}


/*
  The control commands to parse are 4 bytes messages in the input stream:
  1. 0xA0 fix value command prefix
  2. 0x01 relay number [ 1, 2, 3]
  3. 0x00 = off, 0x01 = on
  4. sum of first 3 bytes as simple checksum

  NB: byte value A0 is no valid second, third or fourth byte,
      so A0 is first message byte in any case! 

  First implementation:
  * read message by message 
  * check for errors and handle errors by error/warning messages
  * call handle_message(relay_num, on_off) just if no errors found
  while true:
    read a byte c
    if c is not 0xA0:
        log warning "read invalid message prefix %x", c
    else:
        read 3 bytes as relay_num, on_of, message_checksum
        if message_checksum is not valid:
            log error "invalid message checksum! Ignoring message"
        else if relay_num is not valid:
            log error "invalid relay_num %d! Ignoring message", relay_num
        else if on_off is not valid:
            log warning "invalid on_of %d for relay_num = %d", on_off, relay_num
	else
            call handle_message(relay_num, on_off)

  Future extensions:
  * allow to control a fourth relay 
    (trivial implementation)
  * allow to control multiple relays in one comman
    (relay 255 = 0xFF as virtual "all relays")
  * as security feature set a timeout to switch of all relays,
       when no command is received for a specified time
       (message: 0xA0 0xFE ms4 checksum, with ms4=0: no timeout,
        ms4>0: timeout after 4 milli seconds * ms4)

   Future implementation:
   // use relay_number -> gpio_num gpio table
   gpio_num_t relay_gpio_map = [0, GPIO_NUM_10, GPIO_NUM_1, GPIO_NUM_0]
   #define MIN_RELAY 1
   #define MAX_RELAY (sizeof(relay_gpio_map)-MIN_RELAY)
*/


// initialize by board_attributes:
uint16_t board_id = 0; // 16 bit network device address 


/* ***********************
 * BEGIN configuration section
 * ***********************
 */
const gpio_num_t relay_gpio_map[] = {
    GPIO_NUM_10,
    GPIO_NUM_1,
    GPIO_NUM_0
};

static int gpio_active_value = 0;  // set this to 0 for active low relays and 1 for active high relays 

const int min_relay_num = 1;
const int max_relay_num = min_relay_num + sizeof(relay_gpio_map)-1;

/* ***********************
 * END configuration section
 * ***********************
 */

static int current_state = -1;

int relay_get_state() {
    return current_state;
}


void switch_gpio_state(uint8_t values, uint8_t mask)
    int idx = 0;
    uint8_t state_bit = 1;
    while (mask) {
        if (mask & 1) {
            if (values & 1) {
                current_state |= state_bit;
            } else {
                current_state &= ~state_bit;
            }

        }    
    }
}


int my_getchar() {
    uint8_t c;

    while (true) {
        int len = usb_serial_jtag_read_bytes(&c, 1, pdMS_TO_TICKS(100));

        if (len == 1) {
            LOGI(TAG, "read %2X", c);
            return c;
        }
    }
}

static void relay_task(void* arg) {
    LOGI(TAG, "relay_control task started");

    while (true) {
        int c = my_getchar();
        
        if (c == EOF) {
            LOGI(TAG, "read EOF");
            return;
        }

        if (c != 0xA0){
            LOGW(TAG, "read %2X, expected was first byte A0", c);
            continue;
        }

        int relay_num = my_getchar();

        if (relay_num < 1 || relay_num > 3){
            LOGE(TAG, "illegal relay_num: %d", relay_num);
            continue;
        }

        int on_off = my_getchar();

        if ( on_off < 0 || on_off > 1 ) {
            LOGE(TAG, "illegal on_off: %d", on_off);
            continue;
        }
        
        int check_sum = my_getchar();
        uint8_t expected_sum = (uint8_t)(0xA0 + relay_num + on_off);

        if(check_sum != expected_sum){
            LOGE(TAG, "illegal check_sum: %2X, expected was: %2X", check_sum, expected_sum);
            continue;
        }

        if relay_num == ALL
        switch_relay_state( 1 << (relay_num-min_relay_num), 1 == on_off );

        int bit_no = relay_num - relay_num_min;
        int values = (on_off ? 1 : 0) << bit_no;
        switch_relay_states(values, 1 << bit_no);
        LOGI(TAG, "switched relay %d to %s", relay_num, on_off ? "on" : "off");        
    }

    fatal_error("relay_control task end");
}


#ifndef UNIT_TEST
extern "C" void app_main() {
    printf("\nStart FreeRTOS implementation: %s\n", APP_NAME);

    current_state = 0; // initialize by no relay active

    // future: read configuration from nvs

    for (uint8_t relay_num = relay_num_min; relay_num < relay_num_max; relay_num++) {
        gpio_num_t gpio_num = relay_gpio_map[relay_num];

        // configure gpio
        gpio_reset_pin(gpio_num);
        gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
        
        // most relais are active low, so switch off by setting high 
        gpio_set_level(RELAY_1, 1);
    }
    
    // initialize uart
    usb_serial_jtag_driver_config_t config = {
        .tx_buffer_size = 256,
        .rx_buffer_size = 256,
    };
    usb_serial_jtag_driver_install(&config);
   
    // create tasks
    xTaskCreatePinnedToCore(
      relay_task,      // function for relay control
      "relay_control", // task name
      4096,            // stack size
      NULL,            // parameters
      12,              // priority
      NULL,            // task handle
      0                // core 0
    );
  }
#endif
