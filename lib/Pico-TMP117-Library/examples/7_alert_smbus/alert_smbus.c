// alert_smbus.c
/**
 * Copyright (c) 2024 breaker
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Example to test SMBus Alert when Alert pin is asserted
// TMP117 asserts alert pin, driving it low, Pico reacts and triggers edge fall interrupt

// WIRING: Choose an available GPIO pin on your RP2040 / Pico Board 
// edit the #define TMP117_ALERT_PIN below, wire this the TMP117 alert pin.
// NOTE: Most TMP117 breakouts have the pull-up resistor necessary to hold the pin high.

#include "tmp117.h"
#include "tmp117_registers.h"
#include "pico/stdlib.h"
#include "pico/printf.h"
#include "hardware/i2c.h"
#include <stdbool.h>
#include <stdint.h>

#define SERIAL_INIT_DELAY_MS 1000 // change to suit serial interface used
//#define TMP117_LED_PIN PICO_DEFAULT_LED_PIN // LED alert pin
#define TMP117_LED_PIN 9 // LED alert pin
#define I2C_KHZ 400 // multiplier for I2C frequency
#define TMP117_I2C_SDA_PIN PICO_DEFAULT_I2C_SDA_PIN // set to a different SDA pin as needed
#define TMP117_I2C_SCL_PIN PICO_DEFAULT_I2C_SCL_PIN // set to a different SCL pin as needed
#define TMP117_ALERT_PIN 7 // Pico GPIO pin for TMP117 ALERT (Interrupt)
//#define WAIT_MS 1000 // extra sleep if needed, avoid a cycle time less than 1 second due to SHE
#define TEMP_HIGH 22.11
#define TEMP_LOW 21.2

// flag for interrupt state
volatile bool alert_asserted = 0;

void configure_gpio_pin(void);

// prototype: Interrupt callback for Alert pin
void alert_callback(uint gpio, uint32_t events);

uint32_t get_conversion_delay(void);

/* - Reading the configuration register clears the High Alert and Low Alert flags
 * - Reading the configuration or temperature result registers clears the Data Ready flag. */

int main() {

    // set up serial and I2C
    stdio_init_all();
    sleep_ms(SERIAL_INIT_DELAY_MS);

    configure_gpio_pin();

    i2c_init(i2c_instance, I2C_KHZ * 1000); // 400 KHz max
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);

    // Enable interrupt on the DataAlert pin
    gpio_set_irq_enabled_with_callback(TMP117_ALERT_PIN, GPIO_IRQ_EDGE_FALL, true, &alert_callback);

    soft_reset(); // soft reset TMP117

    set_alert_pin(ALERT_PIN_ALERT_FLAGS); // ensure alert pin reflects Alert flags status

    // choices; AVG_NONE, AVG_8, AVG_32, AVG_64
    set_averaging_mode(AVG_64);
    // choices; CONV_15_5_MS, CONV_125_MS, CONV_250_MS, CONV_500_MS, CONV_1_S, CONV_4_S, CONV_8_S, CONV_16_S
    set_conversion_cycle(CONV_4_S); // e.g. CONV_4_S = 4 seconds

    // get conversion delay based on conversion cycle and averaging bits
    uint32_t delay_ms = get_conversion_delay();

    set_high_limit(TEMP_HIGH); // set high limit (THigh_Limit register 02h)
    set_low_limit(TEMP_LOW); // set low limit (TLow_Limit register 03h)

    // set continuous conversion mode if EEPROM has been set to shutdown mode
    uint8_t conversion_mode = get_conversion_mode();
    if (conversion_mode != CC)
        set_continuous_conversion_mode();
    
    printf("\nExample using SMBus Alert function\n");
    printf("Checking every %.1f seconds for high temp of %.2f °C or low temp of %.2f °C\n\n", delay_ms / 1000.0, TEMP_HIGH, TEMP_LOW);

    while(1) {

    volatile uint8_t result = 2; // set an invalid initial value

    do {
        sleep_ms(delay_ms);
    } while(!alert_asserted);

    // After the alert pin asserts, send the SMBus Alert Response before reading the configuration register
    result = send_smbus_alert();

    // A read of the configuration register to deassert Alert pin
    read_register(TMP117_CONFIGURATION);

    alert_asserted = 0;

    gpio_put(TMP117_LED_PIN, 0);

    // read temperature
    int temp = read_temp_raw() * 100 >> 7;
    // Display the temperature in degrees Celsius, formatted to show two decimal places.
    printf("Temperature: %d.%02d °C\t", temp / 100, (temp < 0 ? -temp : temp) % 100);

    // Also, convert to Fahrenheit and display on the same line (uses floating point math).
    //float temp_float = temp / 100.0;
    //printf("Temperature: %d.%02d °C \t%.2f °F\t", temp / 100, (temp < 0 ? -temp : temp) % 100, calc_temp_fahrenheit(temp_float));

    // extract LSB for high or low flag
    result &= 0x01;

    // TODO: no alert logic not needed
    if (result == 1)
        printf("high alert!\n");
    else if (result == 0)
        printf("low alert!\n");
    else
        printf("no alert. result %d\n", result);

    #ifdef WAIT_MS
    sleep_ms(WAIT_MS); // extra sleep if needed
    #endif

    } // forever loop closing curly brace

    while (1) {
        tight_loop_contents();
    }

} // end main

void configure_gpio_pin(void) {
    gpio_init(TMP117_ALERT_PIN);            // Initialize the GPIO
    gpio_set_dir(TMP117_ALERT_PIN, GPIO_IN); // Set as input
    //gpio_pull_up(TMP117_ALERT_PIN);         // Enable internal pull-up resistor
    gpio_set_input_enabled(TMP117_ALERT_PIN, true); // Enable input buffer
    gpio_init(TMP117_LED_PIN);
    gpio_set_dir(TMP117_LED_PIN, GPIO_OUT);
    gpio_put(TMP117_LED_PIN, 0);
}

// Interrupt callback for Alert pin (high or low alert)
void alert_callback(uint gpio, uint32_t events) {
    if (gpio == TMP117_ALERT_PIN && events == GPIO_IRQ_EDGE_FALL) {
        alert_asserted = 1;
        gpio_put(TMP117_LED_PIN, 1);  // Optionally turn on LED when Alert pin asserts
    }
}

// Determine conversion delay based on the conversion cycle time and averaging mode bits
// See Table 7-7 of the datasheet
uint32_t get_conversion_delay(void) {

    // Read configuration register
    uint16_t configuration_register = read_register(TMP117_CONFIGURATION);

    // extract CONV[2:0] and AVG[1:0] bit values of configuration register
    uint8_t conv = (configuration_register >> 7) & 0x07; // Bits 9, 8, 7
    uint8_t avg = (configuration_register >> 5) & 0x03; // Bits 6 and 5

    // Conversion time table in ms of datasheet Table 7-7 (16 is in place of 15.5)
    const uint32_t conversion_cycle_times[8][4] = {
        {16, 125, 500, 1000},
        {125, 125, 500, 1000},
        {250, 250, 500, 1000},
        {500, 500, 500, 1000},
        {1000, 1000, 1000, 1000},
        {4000, 4000, 4000, 4000},
        {8000, 8000, 8000, 8000},
        {16000, 16000, 16000, 16000}
    };

    // Return conversion time based on AVG and CONV values
    return conversion_cycle_times[conv][avg];
}