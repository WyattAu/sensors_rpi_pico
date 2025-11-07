// set_alert_mode_temp_limits.c
/**
 * Copyright (c) 2024 breaker
 * Based on code from:
 * - Raspberry Pi Pico SDK (c) 2020 Raspberry Pi (Trading) Ltd. (BSD-3-Clause License)
 * - SparkFun TMP117 Arduino Library (c) 2016 SparkFun Electronics (originally MIT, relicensed BSD-3-Clause)
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Original SparkFun TMP117 Arduino Library file: Example5_SetAlertModeTemperatureLimits.ino

#include "tmp117.h"
#include "tmp117_registers.h"
#include "hardware/watchdog.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/printf.h"
#include "pico/stdio.h"
#include <stdbool.h>
#include <stdint.h>

#define SERIAL_INIT_DELAY_MS 1000 // change to suit serial interface used
#define TMP117_I2C_SDA_PIN PICO_DEFAULT_I2C_SDA_PIN // set to a different SDA pin as needed
#define TMP117_I2C_SCL_PIN PICO_DEFAULT_I2C_SCL_PIN // set to a different SCL pin as needed
#define DELAY_MS 1000
#define EXTRA_DELAY_MS 1000 // for the optional extra delay
#define RUNS 100 // how many times to display the temp and alert status

volatile bool input_available = false; // Flag to indicate if input is available
volatile char input_char = 0; // Variable to store the input character
volatile bool mode; // variable to store therm/alert mode 0/1
volatile bool alert_pin;

// Declare function prototypes for character input functions
void inputCallback();
char getInputChar();

// Declare function prototypes for state-handler functions
void mainMenu();
void toggleTnA();
void inputHighLimit();
void inputLowLimit();
void runSensor(void);
void exitMenu(void);
void toggleAlertPin();

// Pointer to the current state-handler function
void (*currentState)() = mainMenu;

int main() {
    // set up serial and I2C
    stdio_init_all();
    sleep_ms(SERIAL_INIT_DELAY_MS);

// ensure board file has I2C pins
#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
#warning alerts example requires a board with I2C pins
    puts("Default I2C pins were not defined");
#else
    i2c_init(i2c_instance, 400 * 1000);
    gpio_set_function(TMP117_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(TMP117_I2C_SCL_PIN, GPIO_FUNC_I2C);
    soft_reset(); // TMP117 software reset; loads EEPROM Power On Reset values
    set_shutdown_mode(); // no conversions until one-shot is set
    
    puts("\n\nSet therm/alert mode and temperature alerts example.");
    puts("---------------------------------------------------------------");

    stdio_set_chars_available_callback(inputCallback, NULL); // Set the input callback

    while (1) {
        currentState();
    }

    return 0;

#endif
}

void inputCallback(void *param) {
    input_available = true; // Set the flag to indicate input is available
    input_char = getchar(); // Read the input character
}

char getInputChar() {
    input_available = false; // Reset the input flag
    return input_char; // Return the input character
}

// Main menu state-handler function
void mainMenu() {
    mode = get_thermalert_mode();
    printf("\n1. Toggle therm/alert mode (bit 4)");
    if (mode == 0)
        printf("\t(0 Alert mode set)\n");
    if (mode == 1)
        printf("\t(1 Therm mode set)\n");
    printf("2. Set temperature high limit\t\t(current: %.2f °C)\n", get_high_limit());
    printf("3. Set temperature low limit\t\t(current: %.2f °C)\n", get_low_limit());
    alert_pin = get_alert_pin();
    printf("4. Toggle alert pin (bit 2)");
    if (alert_pin == 0)
        printf("\t\t(0 alert flag mode set)\n");
    if (alert_pin == 1)
        printf("\t\t(1 data ready flag mode set)\n");
    printf("5. Read Temperature\n");
    printf("6. Exit (reboot)\n");

    int choice;
    printf("\nEnter your choice (do not press Enter): ");
    while (!input_available) {
        sleep_ms(20); // Wait for input to be available
    }

    choice = getInputChar() - '0'; // Convert char to int

    switch (choice) {
        case 1:
            currentState = toggleTnA;
            break;
        case 2:
            currentState = inputHighLimit;
            break;
        case 3:
            currentState = inputLowLimit;
            break;
        case 4:
            currentState = toggleAlertPin;
            break;
        case 5:
            currentState = runSensor;
            break;
        case 6:
            currentState = exitMenu;
            break;
        default:
            printf("\nInvalid choice. Please try again.\n");
            break;
    }
} // end mainMenu()

// toggle bit 4 of config therm / alert mode
//      1: Therm mode
//      0: Alert mode
void toggleTnA() {
    //mode = get_thermalert_mode();
    if (mode == 0) {
        set_thermalert_mode(1);
        currentState = mainMenu;
    }
    if (mode == 1) {
        set_thermalert_mode(0);
        currentState = mainMenu;
    }
}

/* ALERT pin select bit.
1: ALERT pin reflects the status of the data ready flag
0: ALERT pin reflects the status of the alert flags */
void toggleAlertPin() {
    if (alert_pin == 0) {
        set_alert_pin(1); // data ready flag
        currentState = mainMenu;
    }
    if (alert_pin == 1) {
        set_alert_pin(0); // alert flag
        currentState = mainMenu;
    }
}

// input high limit value
void inputHighLimit(void) {
    float highTemp = 0;
    printf("\nInput high limit (-256°C to 255.98°C): ");
    if (scanf("%f", &highTemp) == 1) {
        if ((highTemp >= -256) && (highTemp <= 255.98)) {
            set_high_limit(highTemp);
            currentState = mainMenu;
        }
        else {
            currentState = inputHighLimit;
        }
    }
    else {
        currentState = exitMenu;
    }
}

// input low limit value
void inputLowLimit(void) {
    float lowTemp = 0;
    printf("\nInput low limit (-256°C to 255.98°C): ");
    if (scanf("%f", &lowTemp) == 1) {
        if ((lowTemp >= -256) && (lowTemp <= 255.98)) {
            set_low_limit(lowTemp);
            currentState = mainMenu;
        }
        else {
            currentState = inputLowLimit;
        }
    }
    else {
        currentState = exitMenu;
    }
}

// run the sensor forever or one or more times as you adjust the code
void runSensor(void) {
    // see tmp117.h for struct TMP117_flags_t (holds bool of data_ready, high_alert, low_alert)
    tmp117_flags_t flags; // Declare a struct to hold the flags

    // get user input for number of temperature readings, use it for the loop
    uint runs = 0;
    printf("\nRun how many reads? (type an integer, then press Enter): ");
    if (scanf("%d", &runs) == 1) {
        for (int i = 0; i < runs; i++) {

    // set one-shot mode before each delay and read for a 1:1 match of temp register and flags
    set_oneshot_mode();

    // avoid a delay < 1 second due to the SHE - Self Heating Effect (see datasheet)
    do {
      sleep_ms(DELAY_MS); // 1s is the longest conversion delay possible with one-shot mode
      flags = get_status_flags(); // Get the latest flag status (thus clearing them)
    } while (!flags.data_ready); // If flags.data_ready is true, data is ready

    // Read the temperature result register and use floating-point math to convert to Celsius
    float temp_celsius = read_temp_celsius();
    // print Celsius temperature to serial, calculate and print Fahrenheit temperature to serial
    printf("\nTemperature: %.2f °C\t%.2f °F\t", temp_celsius, calc_temp_fahrenheit(temp_celsius));

    if (flags.low_alert) {
      printf("Low Alert");
    }
    else if (flags.high_alert) {
      printf("High Alert");
    }
    else {
      printf("No Alert");
    }
    // optional extra delay between reads
    //sleep_ms(EXTRA_DELAY_MS);
    }
    }
    else {
        printf("\nREBOOT!");
        sleep_ms(500);
        watchdog_reboot(0, 0, 0);  // Reboot the Pico
    }

    // return to main menu after one or more conversions
    printf("\n"); // spacer
    currentState = mainMenu;
}

// Exit menu state-handler function
void exitMenu(void) {
    printf("\nREBOOT!");
    sleep_ms(500);
    watchdog_reboot(0, 0, 0);  // Reboot the Pico
}