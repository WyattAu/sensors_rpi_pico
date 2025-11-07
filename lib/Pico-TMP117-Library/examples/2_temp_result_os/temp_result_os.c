// temp_result_os.c
/**
 * Copyright (c) 2024 breaker
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* This library assumes I2C address 0x48, use set_address() to change.
   The possible i2C addresses for TMP117 are; 0x48, 0x49, 0x4A, 0x4B */

/* This example will read the raw temperature result register, cast to an integer.
   The printing of Celsius is a no-float temperature reading using Q notation.
   The conversion to Fahrenheit uses floating point math, unfortunately */

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
#define TMP117_DATA_DELAY_MS 1000 // used while waiting for the one-shot conversion to complete
#define TMP117_CONVERSION_DELAY_MS 125 // default 125ms (AVG[1:0] 01)

// variable for checking I2C frequency
int frequency = 0;

void check_status(void);
void check_i2c(void);

int main(void) {
    // initialize chosen interface
    stdio_init_all();
    // a little delay to ensure stability
    sleep_ms(SERIAL_INIT_DELAY_MS);
    // message about the program, thus testing serial comms
    printf("\nOne-shot conversion mode temperature reading.\n");

    // Selects I2C instance (i2c0 is set as default in the tmp117.c)
    //tmp117_set_instance(i2c1); // change to i2c1 as needed
    // initialize I2C put return into variable freq
    frequency = i2c_init(i2c_instance, 400 * 1000); // TMP117 400 kHz max.
    // configure the GPIO pins for I2C
    gpio_set_function(TMP117_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(TMP117_I2C_SCL_PIN, GPIO_FUNC_I2C);
    // call a function to check if i2c is running, print error if no SCL > 0
    check_i2c();
    // uncomment below to set I2C address other than 0x48 (e.g., 0x49)
    //tmp117_set_address(0x49);
    // check if TMP117 is on the I2C bus at the address specified
    check_status();
    // TMP117 software reset; loads EEPROM Power On Reset values
    soft_reset();
    // Set TMP117 to shutdown mode
    set_shutdown_mode();
    // Set TMP117 to one-shot mode
    set_oneshot_mode();
    
    // wait for the conversion to complete, with correct delay it should be 1 iteration
    do {
        sleep_ms(TMP117_DATA_DELAY_MS);
    } while (!data_ready());

    // Convert raw register value to temperature in °C with 2 decimal places
    int temp = read_temp_raw() * 100 >> 7;

    // Display the temperature in degrees Celsius, formatted to show decimal places.
    printf("Temperature: %d.%02d °C\n", temp / 100, (temp < 0 ? -temp : temp) % 100);

    while (1) {
        tight_loop_contents();
    }

    return 0;
}

// check if TMP117 is at the specified address and has correct device ID.
void check_status(void) {
    uint8_t address = tmp117_get_address();
    int status = begin();
    //if (status == TMP117_OK)
    //    printf("TMP117 found at address 0x%02X | I2C frequency %dkHz\n",address,frequency/1000);  // TMP117 is found
    //else if (status == PICO_ERROR_GENERIC) {
    if (status == PICO_ERROR_GENERIC) {
        printf("No I2C device found at address 0x%02X\n",address);  // No device found
        while(1);  // Halt execution if no device is found
    }
    else if (status == TMP117_ID_NOT_FOUND) {
        printf("Non TMP117 I2C device found at address 0x%02X\n",address);  // Wrong device found
        while(1);  // Halt execution if a different device is found
    }
}

// check if I2C is running
void check_i2c(void) {
    if (frequency <= 0) {
        printf("I2C has no clock.\n");
        while(1);
    }
}
