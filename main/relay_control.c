#include "build.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <driver/usb_serial_jtag.h>
#include <nvs_flash.h>
#include <nvs.h>

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

  Current implementation:
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
  * handle A0 FE 00 9E to set gpio_active_level = 0,
       and A0 FE 01 9F to set gpio_active_level = 1

  Future extensions:
  * as security feature set a timeout to switch of all relays,
       when no command is received for a specified time
       (message: 0xA0 0xFE ms4 checksum, with ms4=0: no timeout,
        ms4>0: timeout after 4 milli seconds * ms4)
*/


// initialize by board_attributes:
uint16_t board_id = 0; // 16 bit network device address 


/* ***************************
 * BEGIN configuration section
 * ***************************
 */

const gpio_num_t relay_gpio_map[] = {
    GPIO_NUM_10,
    GPIO_NUM_1,
    GPIO_NUM_0
};

static int gpio_active_level = 1;  // set this to 0 for active low relays and 1 for active high relays 

const int min_relay_num = 1; // may be set to 0 or 1

/* *************************
 * END configuration section
 * *************************
 */

#ifdef UNIT_TEST
static uint32_t test_gpio_state = 0; // 
uint16_t test_get_gpio_state(void) { return test_gpio_state; }
static inline void relay_gpio_write(gpio_num_t gpio_num, uint8_t level) {
    gpio_set_level(gpio_num, level);
    if (level)
        test_gpio_state |= (1 << gpio_num);
    else
        test_gpio_state &= ~(1 << gpio_num);
}
#else
#define relay_gpio_write(gpio_num, level) gpio_set_level(gpio_num, level)
#endif

#define NUM_RELAYS (sizeof(relay_gpio_map)/sizeof(relay_gpio_map[0]))

const int max_relay_num = min_relay_num + NUM_RELAYS - 1;
const relay_bits_t MASK_ALL = (relay_bits_t)((1ul << NUM_RELAYS) - 1);

static relay_bits_t current_state = 0;


void switch_relay_states(relay_bits_t values, relay_bits_t mask) {
    relay_bits_t state_bit = 1;
    int relay_num = min_relay_num;
    while (mask) {
        if (mask & 1) {
            gpio_num_t gpio_num = relay_gpio_map[relay_num - min_relay_num];
            if (values & 1) {
                current_state |= state_bit;
                relay_gpio_write(gpio_num, gpio_active_level);  
            } else {
                current_state &= ~state_bit;
                relay_gpio_write(gpio_num, 1 - gpio_active_level);       
            }
        }
        mask = mask >> 1;
        values = values >> 1;
        relay_num++;
        state_bit <<= 1;
    }
}


int usb_serial_getchar() {
    /* this is the default getchar function to read input from usb device */

    uint8_t c;

    while (true) {
        int len = usb_serial_jtag_read_bytes(&c, 1, pdMS_TO_TICKS(100));

        if (len == 1) {
            LOGI(TAG, "read %2X", c);
            return c;
        }
    }
}


#define NVS_NAMESPACE "relay"
#define NVS_KEY_ACTIVE_LEVEL "active_level"

static void load_gpio_active_level(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);

    if (err == ESP_OK) {
        int32_t level = 1; // default

        err = nvs_get_i32(handle, NVS_KEY_ACTIVE_LEVEL, &level);
        if (err == ESP_OK) {
            gpio_active_level = (int)level;
        }

        nvs_close(handle);
    }

    LOGI(TAG, "loaded gpio_active_level=%d", gpio_active_level);
}

static void save_gpio_active_level(int level)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);

    if (err == ESP_OK) {
        nvs_set_i32(handle, NVS_KEY_ACTIVE_LEVEL, level);
        nvs_commit(handle);
        nvs_close(handle);
    }

    LOGI(TAG, "saved gpio_active_level=%d", level);
}

void set_relay_active_level(int level) { // level might be 0 or 1
    // todo: store gpio_active_level in nva
    if (level == 0 || level == 1) {
        gpio_active_level = level;
        save_gpio_active_level(level);
    } else
        LOGE(TAG, "illegal gpio_active_level %d", level);
}

static int (*input_getchar)(void) = &usb_serial_getchar; // pointer to current getchar function

void set_getchar_function(int (*func)(void)) {
    if (func == NULL)
        func =  &usb_serial_getchar;
    input_getchar = func;
}


static void relay_task(void* arg) {
    /* main relay control
     * reading and parsing input
     */
    LOGI(TAG, "relay_control task started");

    while (true) {
        int c = input_getchar();
        
        if (c == EOF) {
            LOGI(TAG, "read EOF");
            return;
        }

        if (c != cmd_prefix) {
            LOGW(TAG, "read %2X, expected was first byte %2X", c, cmd_prefix);
            continue;
        }

        int relay_cmd = input_getchar(); // relay number or command 0xFF, 0xFE

        if ((relay_cmd < min_relay_num || relay_cmd > max_relay_num) &&
            (relay_cmd != cmd_relay_all) && (relay_cmd != cmd_set_gpio_active_level)){
            LOGE(TAG, "illegal relay_cmd: %d (is no command nor a valid relay number)", relay_cmd);
            continue;
        }

        int on_off = input_getchar();

        if ( on_off < 0 || on_off > 1 ) {
            LOGE(TAG, "illegal on_off: %d", on_off);
            continue;
        }
        
        int check_sum = input_getchar();
        relay_cmd_t expected_sum = (relay_cmd_t)(cmd_prefix + relay_cmd + on_off);

        if ( check_sum != expected_sum ) {
            LOGE(TAG, "illegal check_sum: %2X, expected was: %2X", check_sum, expected_sum);
            continue;
        }

        if (relay_cmd == cmd_relay_all) {
            switch_relay_states( on_off ? MASK_ALL : 0, MASK_ALL );
            LOGI(TAG, "switched all relays to %s", on_off ? "on" : "off");        
        } else if (relay_cmd == cmd_set_gpio_active_level) {
            set_relay_active_level(on_off);
            switch_relay_states( 0, MASK_ALL );  // switch off all relays!
            LOGI(TAG, "set relay active level to %d, switch off all relays", on_off);        
        } else {
            // relay_cmd is a relay number of the relay to switch 
            int bit_no = relay_cmd - min_relay_num;
            int values = (on_off ? 1 : 0) << bit_no;
            switch_relay_states(values, 1 << bit_no);
            LOGI(TAG, "switched relay %d to %s", relay_cmd, on_off ? "on" : "off");        
        }
    }

    fatal_error("relay_control task end");
}


STATIC void init_relays(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // future: read configuration from nvs:
    // gpio_active_level, min_relay_num, max_relay_num, relay_gpio_map

    current_state = 0; // initialize by no relay active

    load_gpio_active_level();
    // set_relay_active_level(1); // initialize gpio_active_level

    for (uint8_t relay_num = min_relay_num; relay_num <= max_relay_num; relay_num++) {
        gpio_num_t gpio_num = relay_gpio_map[relay_num - min_relay_num];

        // configure gpio
        gpio_reset_pin(gpio_num);
        gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);

        relay_gpio_write(gpio_num, 1 - gpio_active_level); // switch off relay
    }

    switch_relay_states(0, MASK_ALL); // switch of all relays,
}


#ifdef UNIT_TEST
relay_bits_t relay_get_states() {
    return current_state;
}

uint8_t relay_get_level(uint8_t relay_num) {
    return get_gpio_level(relay_gpio_map[relay_num - min_relay_num]);
}

uint8_t get_relay_active_level(void);
#endif

#ifndef UNIT_TEST
void app_main() {
    printf("\nStart FreeRTOS implementation: %s\n", APP_NAME);

    init_relays();

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
