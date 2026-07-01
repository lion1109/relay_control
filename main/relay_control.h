#pragma once


// This header defines data structures and allows access by unit tests.

#ifdef UNIT_TEST
#pragma message "compiling TEST, non production"
#define STATIC 
#define EXTERN extern 
#else
#pragma message "compiling non TEST, production"
#define STATIC static
#define EXTERN static 
#endif

// definiton of global data structures for implementation and unit tests  

typedef uint8_t relay_cmd_t;  // relay command bytes
const relay_cmd_t cmd_prefix = 0xA0; // prefix introducing a relay command 
const relay_cmd_t cmd_relay_all = 0xff; // command to switch all relay with one command
const relay_cmd_t cmd_set_gpio_active_level = 0xfe; // command to set the gpio_active_level

typedef uint8_t relay_bits_t; // for maximum 8 relays, set to uint16/32_t for 16/32 relays

typedef int (*getchar_func_ptr)(void); // pointer to getchar function
void set_getchar_function(int (*func)(void)); // set new getchar function, set_getchar_function(NULL) resets to default.

#ifdef UNIT_TEST
void init_relays(void);

relay_bits_t relay_get_states(); // returns a bitset with on_off states of all relays
uint8_t relay_get_level(uint8_t relay_num); // returns gpio output voltage: 0=0V, 1=3.3V

void set_relay_active_level(int level); // level might be 0 or 1
uint8_t get_relay_active_level(void);
#endif
