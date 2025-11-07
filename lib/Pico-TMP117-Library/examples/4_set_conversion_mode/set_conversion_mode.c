// set_conversion_mode.c
/**
 * Copyright (c) 2024 breaker
 * Based on code from:
 * - Raspberry Pi Pico SDK (c) 2020 Raspberry Pi (Trading) Ltd. (BSD-3-Clause License)
 * - SparkFun TMP117 Arduino Library (c) 2016 SparkFun Electronics (originally MIT, relicensed BSD-3-Clause)
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Original SparkFun TMP117 Arduino Library file: Example4_SetConversionMode.ino

/*
bit 11:10 field MOD[1:0]
*** Set conversion mode. ***
00: Continuous conversion (CC)
01: Shutdown (SD)
10: Continuous conversion (CC), Same as 00 (reads back = 00)
11: One-shot conversion (OS)
The MOD1 bit cannot be stored in EEPROM
*/

// Get and set the device functional mode, optionally run the sensor.

#include "tmp117.h"
#include "tmp117_registers.h"
#include "pico/stdlib.h"
#include "pico/printf.h"
#include "hardware/i2c.h"
#include <stdbool.h>
#include <stdint.h>

#define SERIAL_INIT_DELAY_MS 1000 // change to suit serial interface used
#define TMP117_I2C_SDA_PIN PICO_DEFAULT_I2C_SDA_PIN // set to a different SDA pin as needed
#define TMP117_I2C_SCL_PIN PICO_DEFAULT_I2C_SCL_PIN // set to a different SCL pin as needed
#define TMP117_CONVERSION_DELAY_MS 1000 // Adjust the delay based on averaging time, conversion cycle time, and preference

volatile bool input_available = false; // Flag to indicate if input is available
volatile char input_char = 0; // Variable to store the input character

// Define function prototypes for character input functions
void inputCallback(void *param);
char getInputChar(void);

void run_sensor(int conv_mode);
void two_bit_binary_string(int value, char* binary_string);

int main() {
  // init selected serial interface
  stdio_init_all();
  sleep_ms(SERIAL_INIT_DELAY_MS);
  // initialize I2C0 (default)
  i2c_init(i2c_instance, 400 * 1000); // 400 kHz
  // set I2C pins - configure the GPIO pins for I2C
  gpio_set_function(TMP117_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(TMP117_I2C_SCL_PIN, GPIO_FUNC_I2C);
  
  soft_reset();

  // Set the input callback
  stdio_set_chars_available_callback(inputCallback, NULL);
  
  printf("\n\n+-------------------------------------------------------+");
  printf("\n|\tTMP117 Example 4: Setting Conversion Modes\t|");

  while (1) {
    volatile int conv_mode = 0;
    volatile int current_mode = get_conversion_mode();
    char current_mode_binary[3]; // Buffer for binary string of AVG mode

    two_bit_binary_string(current_mode, current_mode_binary);

    printf("\n+-------------------------------------------------------+");
    printf("\n|\tConversion Modes: MOD[1:0]\t\t\t|");
    printf("\n|\t(1) 00: Continuous conversion (CC)\t\t|");
    printf("\n|\t(2) 01: Shutdown (SD)\t\t\t\t|");
    printf("\n|\t(3) 11: One-shot conversion (OS)\t\t|");
    printf("\n|\t(0) Read temperature (OS or CC mode)\t\t|");
    printf("\n+-------------------------------------------------------+");
    printf("\nCurrent conversion mode: %s ",current_mode_binary);
    printf("| Select menu item 0-3: ");

        while (!input_available) {
        sleep_ms(10); // Wait for input to be available
    }
    conv_mode = getInputChar() - '0'; // Convert char to int

      switch (conv_mode) {
      case 0:
        if (current_mode == 1) {
          printf(" (SD) Shutdown Mode set");
          break;
        }
        else if (current_mode == 3) {
          set_oneshot_mode();
          printf("\n\n");
          run_sensor(current_mode);
        }
        else {
          printf("\n\n");
          run_sensor(current_mode);
        }
        break;
      case 1:
        set_continuous_conversion_mode(); // MOD = 00
        printf(" (CC) Continuous Conversion Mode set");
        break;
      case 2:
        set_shutdown_mode(); // MOD = 01
        printf(" (SD) Shutdown Mode set");
        break;
      case 3:
        set_oneshot_mode(); // MOD = 11
        printf(" (OS) One Shot Mode set");
        break;
      default:
        printf(" Invalid conversion mode, please enter 1, 2, or 3");
        sleep_ms(500);
        break;
      }
    }
  return 0;
} // end main

void inputCallback(void *param) {
    input_available = true; // Set the flag to indicate input is available
    input_char = getchar(); // Read the input character
}

char getInputChar(void) {
    input_available = false; // Reset the input flag
    return input_char; // Return the input character
}

// run sensor; once or forever
void run_sensor(int conv_mode) {
    do {

        uint count = 0;

        do {
            ++count;
            sleep_ms(TMP117_CONVERSION_DELAY_MS);
        } while (!data_ready()); // If flags.data_ready is true, data is ready

        /* 1) typecast temp_result register to integer, converting from two's complement
         * 2) Multiply by 100 to scale the temperature (i.e. 2 decimal places)
         * 3) Shift right by 7 to account for the TMP117's 1/128 resolution (Q7 format), converting to degrees Celsius */
        int16_t temp = read_temp_raw() * 100 >> 7;
        float temp_float = temp / 100.0;
        printf("Temperature: %d.%02d °C\t%.2f °F\n", temp / 100, (temp < 0 ? -temp : temp) % 100, calc_temp_fahrenheit(temp_float));
        
        // floating point functions are also available for converting temp_result to Cesius or Fahrenheit
        //rintf("Temperature: %.2f °C\t%.2f °F\n", read_temp_celsius(), read_temp_fahrenheit());

    } while (conv_mode == CC);
}

// Function to convert an integer to an 2-bit binary string
void two_bit_binary_string(int value, char* binary_string) {
    for (int i = 1; i >= 0; i--) {
        binary_string[1 - i] = ((value >> i) & 1) ? '1' : '0';
    }
    binary_string[2] = '\0'; // Null-terminate the string
}