// set_converstion_avg_cycle.c
/**
 * Copyright (c) 2024 breaker
 * Based on code from:
 * - Raspberry Pi Pico SDK (c) 2020 Raspberry Pi (Trading) Ltd. (BSD-3-Clause License)
 * - SparkFun TMP117 Arduino Library (c) 2016 SparkFun Electronics (originally MIT, relicensed BSD-3-Clause)
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Original SparkFun TMP117 Arduino Library file: Example6_SetConversionCycleTime.ino

/* NOTE: the device requires 1.5 ms to power up before conversions can begin.

The EEPROM factory default is 8 temperature conversions averaged per second.
Averaging can be used in both the continuous conversion mode and one-shot mode
See section 7.3.2 Averaging in the datasheet for details

01 Configuration Register BIT 6:5 FIELD AVG[1:0] R/W RESET 01 
Conversion averaging modes. Determines the number of
conversion results that are collected and averaged before
updating the temperature register. The average is an
accumulated average and not a running average.
00: No averaging
01: 8 Averaged conversions
10: 32 averaged conversions
11: 64 averaged conversions

01 Configuration Register BIT 9:7 FIELD CONV[2:0] R/W RESET 100
Conversion cycle bit. See Table 7-7 for the standby time between conversions.

Table 7-7. Conversion Cycle Time in CC Mode
CONV[2:0]   AVG[1:0] = 00   AVG[1:0] = 01   AVG[1:0] = 10   AVG[1:0] = 11
averaging   0 conversions   8 conversions   32 conversions  64 conversions
000         15.5 ms         125 ms          500 ms          1 s
001         125 ms          125 ms          500 ms          1 s
010         250 ms          250 ms          500 ms          1 s
011         500 ms          500 ms          500 ms          1 s
100         1 s             1 s             1 s             1 s
101         4 s             4 s             4 s             4 s
110         8 s             8 s             8 s             8 s
111         16 s            16 s            16 s            16 s

If the time to complete the conversions needed for a given averaging setting
is higher than the conversion setting cycle time, there will be no stand by
time in the conversion cycle.

  CONV = Conversion Cycle Bit
  AVG  = Conversion Averaging Mode
*/

#include "tmp117.h"
#include "tmp117_registers.h"
#include "hardware/watchdog.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/printf.h"
#include "pico/stdio.h"
#include "pico/time.h"
#include <stdbool.h>
#include <stdint.h>

#define SERIAL_INIT_DELAY_MS 1000 // change to suit serial interface used
#define TMP117_I2C_SDA_PIN PICO_DEFAULT_I2C_SDA_PIN // set to a different SDA pin as needed
#define TMP117_I2C_SCL_PIN PICO_DEFAULT_I2C_SCL_PIN // set to a different SCL pin as needed

volatile bool input_available = false; // Flag to indicate if input is available
volatile char input_char = 0; // Variable to store the input character

// Define function prototypes for character input functions
void inputCallback(void *param);
char getInputChar(void);

// Define function prototypes for state-handler functions
void mainMenu(void);
void inputAVG(void);
void inputCONV(void);
void exitMenu(void);

// Define function prototype for function to convert an integer to an 8-bit binary string
void conv_to_binary_string(int value, char* binary_string);
void avg_to_binary_string(int value, char* binary_string);

// Function to check and display the conversion cycle based on AVG and CONV bits
const char* display_conversion_time(void);

// Function to convert the AVG value to a descriptive string
const char* convert_avg_mode(int avg_mode);

// Pointer to the current state-handler function
void (*currentState)() = mainMenu;

int main() {
    // set up serial and I2C
    stdio_init_all();
    sleep_ms(SERIAL_INIT_DELAY_MS);

    // uncomment below to set I2C address other than 0x48 (e.g., 0x49)
    //tmp117_set_address(0x49);

    // Selects I2C instance (i2c_default is set as default in the tmp117.c)
    //tmp117_set_instance(i2c1); // change to i2c1 as needed

    i2c_init(i2c_instance, 400 * 1000);
    gpio_set_function(TMP117_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(TMP117_I2C_SCL_PIN, GPIO_FUNC_I2C);
        
    printf("\n\nExample 6: Setting Conversion Cycle Time and Averaging modes");

    stdio_set_chars_available_callback(inputCallback, NULL); // Set the input callback

    while (1) {
        currentState();
    }

    return 0;
}

void inputCallback(void *param) {
    input_available = true; // Set the flag to indicate input is available
    input_char = getchar(); // Read the input character
}

char getInputChar(void) {
    input_available = false; // Reset the input flag
    return input_char; // Return the input character
}

// Main menu state-handler function
void mainMenu(void) {
    uint8_t cycleBit = 0;  // variable to store the CONV bits state
    uint8_t avg_mode = 0; // variable to store the AVG bits state

    avg_mode = get_conversion_averaging_mode();
    cycleBit = get_conversion_cycle_bit();

    char avg_modeBinary[3]; // Buffer for binary string of AVG mode
    char cycleBitBinary[4]; // Buffer for binary string of CYCLE bit

    // Convert decimal values to binary strings
    avg_to_binary_string(avg_mode, avg_modeBinary);
    conv_to_binary_string(cycleBit, cycleBitBinary);

    printf("\n-------------------------------------------------------------------------");
    // Print binary and decimal values of bits 6-5
    printf("\nAVG  [1:0]\tbinary:  %s\tdecimal: %d", avg_modeBinary, avg_mode);
    // Print the descriptive string for AVG mode
    printf("\t%s", convert_avg_mode(avg_mode));
    // Print binary and decimal values of bits 9-7
    printf("\nCONV [2:0]\tbinary: %s\tdecimal: %d", cycleBitBinary, cycleBit);
    // print the conversion cycle time
    printf("\t%s cycle time", display_conversion_time());
    printf("\n-------------------------------------------------------------------------");
    // Display menu choices
    printf("\n1. Select averaging mode");
    printf("\n2. Select conversion cycle time");
    printf("\n3. Reboot Pico");

    int choice;
    printf("\nEnter your choice 1-3: ");
    while (!input_available) {
        sleep_ms(20); // Wait for input to be available
    }

    choice = getInputChar() - '0'; // Convert char to int

    switch (choice) {
        case 1:
            currentState = inputAVG;
            break;
        case 2:
            currentState = inputCONV;
            break;
        case 3:
            currentState = exitMenu;
            break;
        default:
            printf("\nInvalid choice. Please try again.");
            sleep_ms(500);
            break;
    }
}

// user inputs AVG mode bits
void inputAVG(void) {
    uint8_t mode = 0;

    printf("\n0 - 3 for the New Averaging Mode: ");
        while (!input_available) {
        sleep_ms(10); // Wait for input to be available
    }

    mode = getInputChar() - '0'; // Convert char to int

    if ((mode >= 0) && (mode <= 3))
    {
      set_averaging_mode(mode);
      currentState = mainMenu;
    }
    else
    {
      printf(" Please Enter a Number 0-3");
    }
}

// user inputs CONV mode bits
void inputCONV(void) {
    uint8_t mode = 0;

    printf("\n0 - 7 for the New Conversion Cycle Time: ");
        while (!input_available) {
        sleep_ms(10); // Wait for input to be available
    }

    mode = getInputChar() - '0'; // Convert char to int

    if ((mode >= 0) && (mode <= 7))
    {
      set_conversion_cycle(mode);
      currentState = mainMenu;
    }
    else
    {
      printf(" Please Enter a Number 0 - 7");
    }
}

// Exit menu state-handler function
void exitMenu(void) {
    printf("\nREBOOT!");
    sleep_ms(500);
    watchdog_reboot(0, 0, 0);  // Reboot the Pico
}

// Function to convert an integer to an 2-bit binary string
void avg_to_binary_string(int value, char* binary_string) {
    for (int i = 1; i >= 0; i--) {
        binary_string[1 - i] = ((value >> i) & 1) ? '1' : '0';
    }
    binary_string[2] = '\0'; // Null-terminate the string
}        

// Function to convert an integer to an 3-bit binary string
void conv_to_binary_string(int value, char* binary_string) {
    for (int i = 2; i >= 0; i--) {
        binary_string[2 - i] = ((value >> i) & 1) ? '1' : '0';
    }
    binary_string[3] = '\0'; // Null-terminate the string
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

// Function to check and display the conversion cycle based on AVG and CONV bits
const char* display_conversion_time(void) {
    // Read configuration register
    uint16_t configReg = read_register(TMP117_CONFIGURATION);

    // Extract AVG and CONV bits
    uint8_t avg = (configReg >> 5) & 0x03; // Bits 6 and 5
    uint8_t conv = (configReg >> 7) & 0x07; // Bits 9, 8, 7

    // Conversion time table with null-terminated strings
    const char* conversionTimes[8][4] = {
        {"15.5ms\0", "125ms\0", "500ms\0", "1s\0"},
        {"125ms\0", "125ms\0", "500ms\0", "1s\0"},
        {"250ms\0", "250ms\0", "500ms\0", "1s\0"},
        {"500ms\0", "500ms\0", "500ms\0", "1s\0"},
        {"1s\0", "1s\0", "1s\0", "1s\0"},
        {"4s\0", "4s\0", "4s\0", "4s\0"},
        {"8s\0", "8s\0", "8s\0", "8s\0"},
        {"16s\0", "16s\0", "16s\0", "16s\0"}
    };

    // Return conversion time based on AVG and CONV values
    return conversionTimes[conv][avg];
}