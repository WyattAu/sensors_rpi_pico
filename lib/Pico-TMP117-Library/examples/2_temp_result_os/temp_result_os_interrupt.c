// temp_result_os_interrupt.c
/**
 * Copyright (c) 2024 breaker
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// TO DO: add OLED or 16x2 character display functionality

/* This library assumes I2C address 0x48, use set_address() to change.
   The possible i2C addresses for TMP117 are; 0x48, 0x49, 0x4A, 0x4B

   This example will read the raw temperature result register,
   cast to an integer to make sure negative values are displayed correctly.
   The printing of Celsius is a no-float temperature reading using Q notation.
   The conversion to Fahrenheit uses floating point math, unfortunately.

   This example shows the use of the Alert pin to reflect the status of the data ready flag
   and interrupt the Pico on the defined alert pin GPIO using an interrupt callback function.

   Reading the configuration register clears the High Alert and Low Alert flags
   Reading the configuration or temperature result registers clears the Data Ready flag.

   WIRING: Choose an available GPIO pin on your RP2040 / RP2350 board
   edit the #define TMP117_ALERT_PIN below, wire this the TMP117 alert pin.
   Either solder a header onto TMP117 or solder 1 wire from INT to Pico board.

   NOTE: Most TMP117 breakouts have the pull-up resistor necessary to hold the pin high. */

#include "tmp117.h"
#include "tmp117_registers.h"
#include "pico/stdlib.h"
#include "pico/printf.h"
#include "hardware/i2c.h"
#include <stdbool.h>
#include <stdint.h>

// Set to desired GPIO pin for TMP117 ALERT (Interrupt)
#define TMP117_ALERT_PIN 7

#define SERIAL_INIT_DELAY_MS 1000 // change to suit serial interface used

// manual GPIO pin assignment for TMP117 LED alert pin
//#define TMP117_LED_PIN 15

// Use board file's definition if not manually defined
#ifndef TMP117_LED_PIN
    #define TMP117_LED_PIN PICO_DEFAULT_LED_PIN
#endif

// I2C pins, override board file
//#define TMP117_I2C_SDA_PIN 8 // set to a different SDA pin as needed
//#define TMP117_I2C_SCL_PIN 9 // set to a different SCL pin as needed

// Use board file's definitions if not manually defined
#ifndef TMP117_I2C_SDA_PIN
    #define TMP117_I2C_SDA_PIN PICO_DEFAULT_I2C_SDA_PIN // Set to default if not defined
#endif

#ifndef TMP117_I2C_SCL_PIN
    #define TMP117_I2C_SCL_PIN PICO_DEFAULT_I2C_SCL_PIN // Set to default if not defined
#endif

#define TMP117_MODE_DELAY_MS 1
#define TMP117_CONVERSION_DELAY_MS 125 // 125ms (AVG[1:0] default 01)

// variable for checking I2C frequency
int frequency = 0;

// data ready flag for interrupt
bool data_ready_flag = 0;

// variable to hold conversion delay
uint32_t delay_ms = 0;

// Interrupt callback for DataAlert pin (data ready)
void data_alert_callback(uint gpio, uint32_t events);

// functions to check I2C on Pico and if I2C connection to TMP117 is OK
void check_i2c(void);
void check_status(void);

// Function to convert the AVG value to a descriptive string
const char* convert_avg_mode(int avg_mode);
// Function to convert an integer to an 2-bit binary string
void avg_to_binary_string(int value, char* binary_string);
// get conversion averaging delay and put it into a variable
uint32_t get_conversion_averaging_delay(uint8_t avg_mode);

int main(void) {
    // initialize chosen interface
    stdio_init_all();
    // a little delay to ensure stability
    sleep_ms(SERIAL_INIT_DELAY_MS);
    // message about the program, thus testing serial comms
    printf("\nOne-shot conversion mode temperature readings using Alert pin\n");

    gpio_init(TMP117_ALERT_PIN);
    gpio_set_dir(TMP117_ALERT_PIN, GPIO_IN);

    #ifdef TMP117_LED_PIN
    gpio_init(TMP117_LED_PIN);
    gpio_set_dir(TMP117_LED_PIN, GPIO_OUT);
    gpio_put(TMP117_LED_PIN, 0);
    #endif

    // Selects I2C instance (i2c_default is set as default in the tmp117.c)
    //tmp117_set_instance(i2c1); // change to i2c1 as needed

    #if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
        #warning temp_result_os_interrupt example requires a board with I2C pins
        puts("I2C pins were not defined");
    #else
    // initialize I2C and put return into variable freq
    frequency = i2c_init(i2c_instance, 400 * 1000); // TMP117 400 kHz max.
    // configure the GPIO pins for I2C
    gpio_set_function(TMP117_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(TMP117_I2C_SCL_PIN, GPIO_FUNC_I2C);
    #endif

    // Enable interrupt on the DataAlert pin
    gpio_set_irq_enabled_with_callback(TMP117_ALERT_PIN, GPIO_IRQ_EDGE_FALL, true, &data_alert_callback);

    // call a function to check if i2c is running, print error if no SCL > 0
    check_i2c();

    // uncomment below to set I2C address other than 0x48 (e.g., 0x49)
    //tmp117_set_address(0x49);

    // check if TMP117 is on the I2C bus at the address specified
    check_status();

    // TMP117 software reset; loads EEPROM Power On Reset values
    soft_reset();

    // put TMP117 in shutdown mode until we need it to run
    set_shutdown_mode();

    // ALERT_PIN_DATA_READY - ALERT pin reflects the status of the data ready flag
    // ALERT_PIN_ALERT_FLAGS - ALERT pin reflects the status of the alert flags
    set_alert_pin(ALERT_PIN_DATA_READY);
    // conversion averaging modes; AVG_NONE = no averaging, AVG_8, AVG_32, AVG_64 (averaged conversions)
    set_averaging_mode(AVG_64); // set averaging mode, see Table 7-6 of datasheet

    uint8_t avg_mode = 0;
    // Adjust delay based on the conversion averaging mode
    avg_mode = get_conversion_averaging_mode();
    delay_ms = get_conversion_averaging_delay(avg_mode);

    char avg_mode_binary[3] = {0}; // Buffer for binary string of AVG mode
    // Convert decimal values to binary strings
    avg_to_binary_string(avg_mode, avg_mode_binary);

    // Print binary values of bits 6 and 5 of configuration register
    printf("AVG[1:0] Conversion averaging mode: %s", avg_mode_binary);
    // Print the descriptive string for AVG mode
    printf(": %s ", convert_avg_mode(avg_mode));
    printf("(conversion delay in ms: %d)\n",delay_ms);

    while(1) {
    // Set TMP117 to one-shot mode
    set_oneshot_mode();
    
    // wait for the conversion to complete, with correct delay it should be 1 iteration
    do {
        sleep_ms(delay_ms);
    } while (!data_ready_flag);

    // Convert raw register value to temperature in °C with 2 decimal places
    int temp = read_temp_raw() * 100 >> 7;
    data_ready_flag = 0;  // set flag to 0 to reflect clear by temperature read

    #ifdef TMP117_LED_PIN
    gpio_put(TMP117_LED_PIN, 0); // turn off LED after temperature is read
    #endif

    // Display the temperature in degrees Celsius, formatted to show decimal places.
    // Also, convert to Fahrenheit and display on the same line (uses floating point math).
    float temp_float = temp / 100.0;
    printf("Temperature: %d.%02d °C \t%.2f °F\n", temp / 100, (temp < 0 ? -temp : temp) % 100, calc_temp_fahrenheit(temp_float));
    }
    
    // The below is if one-shot mode is set to run only one or more times instead of forever
    while (1) {
        tight_loop_contents();
    }

    return 0;
}

// Interrupt callback for DataAlert pin (data ready)
void data_alert_callback(uint gpio, uint32_t events) {
    if (gpio == TMP117_ALERT_PIN && events == GPIO_IRQ_EDGE_FALL) {
        data_ready_flag = 1;
        #ifdef TMP117_LED_PIN
        gpio_put(TMP117_LED_PIN, 1);  // Optionally turn on LED when data is ready
        #endif
    }
}

// check if TMP117 is at the specified address and has correct device ID.
void check_status(void) {
    uint8_t address = tmp117_get_address();
    int status = begin();

    switch (status) {
        case TMP117_OK:
            printf("TMP117 found at address 0x%02X, I2C frequency %dkHz\n", address, frequency / 1000);
            break;

        case PICO_ERROR_TIMEOUT:
            printf("I2C timeout reached after %u microseconds\n", SMBUS_TIMEOUT_US);
            while (1) {
                tight_loop_contents();  // Halt execution if timeout occurs
            }
            break;

        case PICO_ERROR_GENERIC:
            printf("No I2C device found at address 0x%02X\n", address);
            while (1) {
                tight_loop_contents();  // Halt execution if no device found
            }
            break;

        case TMP117_ID_NOT_FOUND:
            printf("Non-TMP117 device found at address 0x%02X\n", address);
            while (1) {
                tight_loop_contents();  // Halt execution if a wrong device is found
            }
            break;

        default:
            printf("Unknown error during TMP117 initialization\n");
            while (1) {
                tight_loop_contents();  // Halt execution for unexpected errors
            }
    }
}

// check if I2C is running
void check_i2c(void) {
    if (frequency == 0) {
        printf("I2C has no clock.\n");
        while(1) {
            tight_loop_contents();  // Halt execution if I2C frequency is zero
        };
    }
}

// Function to convert the AVG value to a descriptive string
const char* convert_avg_mode(int avg_mode) {
    switch (avg_mode) {
        case 0: return "No averaging\0";
        case 1: return "8 averaged conversions\0";
        case 2: return "32 averaged conversions\0";
        case 3: return "64 averaged conversions\0";
        default: return "Unknown\0";
    }
}

// Function to convert an integer to an 2-bit binary string
void avg_to_binary_string(int value, char* binary_string) {
    for (int i = 1; i >= 0; i--) {
        binary_string[1 - i] = ((value >> i) & 1) ? '1' : '0';
    }
    binary_string[2] = '\0'; // Null-terminate the string
}

// get delay based on averaging mode
uint32_t get_conversion_averaging_delay(uint8_t avg_mode) {
    // Adjust delay based on the conversion averaging mode
    avg_mode = get_conversion_averaging_mode();

    switch (avg_mode) {
        case 0:
            return 16;  // 15.5 ms rounded
            break;
        case 1:
            return 125;
            break;
        case 2:
            return 500;
            break;
        case 3:
            return 1000;
            break;
        default:
            return TMP117_CONVERSION_DELAY_MS; // fallback to default if unknown mode
            break;
    }
}
