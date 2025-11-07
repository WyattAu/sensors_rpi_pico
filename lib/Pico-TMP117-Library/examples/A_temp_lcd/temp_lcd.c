// temp_lcd.c
/**
 * Copyright (c) 2024 breaker
 * Based on code from:
 * - Raspberry Pi Pico SDK (c) 2020 Raspberry Pi (Trading) Ltd. (BSD-3-Clause License)
 * - SparkFun TMP117 Arduino Library (c) 2016 SparkFun Electronics (originally MIT, relicensed BSD-3-Clause)
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
// based on SparkFun TMP117 Arduino Library file: Example1_BasicReadings.ino

/* HD44780 16x2 Character LCD Display (4-bit mode)
 * Wiring for a 16-pin, 5V DC Character LCD using the HD44780 Controller
 *
 * Connections:
 * Pico <--> LCD
 * - VBUS       <--> 5V DC rail
 * - GND        <--> GND rail
 * - GND rail   <--> LCD pins: 16 (backlight cathode), 5 (RW), 1 (logic GND), potentiometer (leg 1)
 * - Potentiometer wiper <--> LCD pin 3 (contrast adjustment, VO)
 * - 5V DC rail <--> LCD pins: 15 (backlight anode), 2 (logic VDD), potentiometer (leg 3)
 * - Pico GPIO pins (user-defined in code) <--> LCD pins:
 *      - RS  (Register Select)
 *      - EN  (Enable)
 *      - D7, D6, D5, D4 (Data lines for 4-bit mode)
 *
 * Notes:
 * 1. This configuration assumes the LCD is used in read-only mode (RW tied to GND).
 *    If bidirectional communication is required, use a voltage divider or logic level 
 *    shifter to safely interface 5V signals with the Pico's 3.3V GPIO.
 * 2. Ensure the backlight is powered correctly to avoid damage.
 * 3. The potentiometer allows contrast adjustment by controlling the voltage on pin 3 (VO).
 */

/* Notes for this new version using the Raspberry Pi Pico/RP2040:
*
* This program will print the temperature in Celsius by casting the raw temperature
* result register to an integer, then scale the Q7 formatted data to two decimal places
* 
* See TI Application Note: How to Read and Interpret Digital Temperature Sensor Output Data
* "Modern sensors, such as the TMP117, offer a full 16 bits of resolution in a Q7 format."
*
* The 16-bit Word of the temp_result register consists of;
* bit 15: sign bit, [14:7] Integer bits, "decimal point", [6:0] Fractional bits
*
* Please see Table 7-7. (Conversion Cycle Time in CC Mode) note: CC = Continuous Conversion
* The factory default Conversion Cycle time is 1 second using 8 averaged conversions,
* therefore this example sets the wait for conversion to 1000 ms.
*
* NOTE: Possible i2C addresses for TMP117 (0x48, 0x49, 0x4A, 0x4B)
* Library assumes the address is 0x48 unless set_address function is used
*
* - Reading the configuration register clears the High Alert and Low Alert flags
* - Reading the configuration or temperature result registers clears the Data Ready flag.
*/

#include "lcd.h"
#include "tmp117.h"
#include "tmp117_registers.h"
#include "pico/stdlib.h"
#include "pico/printf.h"
#include "hardware/i2c.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define SERIAL_INIT_DELAY_MS 1000 // adjust as needed to mitigate garbage characters after serial interface is started
#define TMP117_I2C_SDA_PIN PICO_DEFAULT_I2C_SDA_PIN // set to a different SDA pin as needed
#define TMP117_I2C_SCL_PIN PICO_DEFAULT_I2C_SCL_PIN // set to a different SCL pin as needed
#define TMP117_OFFSET_VALUE -130.0f  // temperature offset in degrees C set by user (try negative values for testing)
#define TMP117_CONVERSION_DELAY_MS 4000 // Adjust the delay based on conversion cycle time and preference

// LCD to GPIO mapping, edit as desired to match your wiring.
#define LCD_RS  10   // Register Select
#define LCD_EN  11   // Enable
#define LCD_D4  18   // Data pin 4
#define LCD_D5  19   // Data pin 5
#define LCD_D6  20   // Data pin 6
#define LCD_D7  21   // Data pin 7

void check_i2c(void);
void check_status(void);
void lcd_msg(void);
uint32_t get_conversion_delay(void);

// variable for checking I2C frequency
uint frequency = 0;

int main(void) {

    // initialize chosen serial interface
    stdio_init_all();
    // a little delay to ensure serial line stability
    sleep_ms(SERIAL_INIT_DELAY_MS);
    // message for serial
    printf("Temperature readings on 16x2 character LCD\n");

    // LCD init and display characters that never change
    lcd_msg();

    // uncomment below to set I2C address other than 0x48 (e.g., 0x49)
    //tmp117_set_address(0x49);

    // Selects I2C instance (i2c_default is set as default in the tmp117.c)
    //tmp117_set_instance(i2c1); // change to i2c1 as needed

    // initialize I2C (default i2c_default) and initialize variable with I2C frequency
    frequency = i2c_init(i2c_instance, 400 * 1000); // TMP117 400 kHz max.

    // configure the GPIO pins for I2C
    gpio_set_function(TMP117_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(TMP117_I2C_SCL_PIN, GPIO_FUNC_I2C);

    // call a function to check and report if i2c is running
    check_i2c();
 
    // check if TMP117 is on the I2C bus at the address specified
    check_status();

    // TMP117 software reset; loads EEPROM Power On Reset values
    soft_reset();

    // uncomment to test with a temperature offset
    //set_temp_offset(TMP117_OFFSET_VALUE);

     // set continuous conversion mode if EEPROM has been set to shutdown mode
    uint8_t conversion_mode = get_conversion_mode();
    if (conversion_mode != 0)
        set_continuous_conversion_mode();

    // test different conversion cycle times, see Table 7-7 of the datasheet
    set_conversion_cycle(CONV_4_S);     // set desired cycle time (avoid <500ms)
    set_averaging_mode(AVG_32);         // set desired averaging mode

    // get conversion delay based on conversion cycle and averaging bits
    uint32_t delay_ms = get_conversion_delay();

    while (1) {

        do {
            sleep_ms(delay_ms);
        } while (!data_ready()); // check if the data ready flag is high

        /* 1) typecast temp_result register to integer, converting from two's complement
           2) Multiply by 100 to scale the temperature (i.e. 2 decimal places)
           3) Shift right by 7 to account for the TMP117's 1/128 resolution (Q7 format) */
        int temp = read_temp_raw() * 100 >> 7;
        int integer = temp / 100;
        int decimal = (temp < 0 ? -temp : temp) % 100;
        float temp_float = temp / 100.0;
        float temp_fahrenheit = calc_temp_fahrenheit(temp_float);
        char buffer[8] = {0}; // Buffer to hold the formatted temperature string

        // Format the temperature in Celsius as "integer.decimal"
        sprintf(buffer, "%d.%02d", integer, decimal); // Ensure two decimal places
        lcd_set_cursor(1, 0);  // Row 1, Col 0
        lcd_print(buffer);     // Print the formatted temperature string to the LCD

        // Format and display the temperature in Fahrenheit
        sprintf(buffer, "%.02f", temp_fahrenheit); // Ensure two decimal places
        lcd_set_cursor(0, 0);  // Row 0, Col 0
        lcd_print(buffer);     // Print the formatted temperature string to the LCD

        // Also print the temperature in Celsius, and Fahrenheit to serial
        printf("Temperature: %d.%02d °C \t%.02f °F\n", integer, decimal, temp_fahrenheit);

        // Or, also print the temperature in degrees Celsius to the serial monitor
        //printf("Temperature: %d.%02d °C\n", integer, decimal);


    }

    return 0;
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

// check if I2C is running on Pico
void check_i2c(void) {
    if (frequency == 0) {
        printf("I2C has no clock.\n");
        while(1) {
            tight_loop_contents();  // Halt execution if I2C frequency is zero
        };
    }
}

// LCD init and display characters that never change
void lcd_msg(void) {

    // Define LCD GPIO pin assignments
    LCD_Config my_lcd_config = {
        .rs_pin = LCD_RS,
        .enable_pin = LCD_EN,
        .data_pins = {LCD_D4, LCD_D5, LCD_D6, LCD_D7}
    };

    // bitmap for custom degree symbol character (same as 0xDF of A00 ROM)
    uint8_t degree_symbol[8] = {0x1C, 0x14, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00};

    // Initialize the LCD with the specified pins
    lcd_pin_init(&my_lcd_config);
    lcd_init();
    // Create custom degree symbol character
    lcd_create_char(0, degree_symbol);
    
    // set cursor and display static text
    lcd_set_cursor(0, 8);  // Row 0, Col 0 (allow for temperature of 7 characters and a space)
    lcd_send_byte(1, 0x00);  // Display custom degree symbol character
    lcd_print("F");
    lcd_set_cursor(1, 8);  // Row 1, Col 8 (allow for temperature of 7 characters and a space)
    //lcd_send_byte(1, 0xDF);  // Display degree symbol using character code (A00 ROM)
    lcd_send_byte(1, 0x00);  // Display custom degree symbol character
    lcd_print("C");
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

