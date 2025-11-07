/**
 * Copyright (c) 2024 breaker
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* temp_time.c
*
* This program uses functions to set the averaging and conversion cycle time bits, then,
* a function uses a table to determine the minimum delay before data is ready.
*
* After the data is ready, we read and print the temperature in Celsius by casting the raw temperature
* result register to an integer, then scale the Q7 formatted data to two decimal places
* 
* See TI Application Note: How to Read and Interpret Digital Temperature Sensor Output Data
* "Modern sensors, such as the TMP117, offer a full 16 bits of resolution in a Q7 format."
*
* The 16-bit Word of the temp_result register consists of;
* bit 15: sign bit, [14:7] Integer bits, "decimal point", [6:0] Fractional bits
*
* Please see Table 7-7. (Conversion Cycle Time in CC Mode) note: CC = Continuous Conversion
*
* NOTE: Possible i2C addresses for TMP117 (0x48, 0x49, 0x4A, 0x4B)
* Library assumes the address is 0x48 unless set_address function is used
*
* To keep it simple, this program does not check for I2C clock nor if the TMP117 is present,
*   see example 0, read_registers.c for how those checks might be coded
*/

#include "tmp117.h"
#include "tmp117_registers.h"
#include "pico/stdlib.h"
#include "pico/printf.h"
#include "hardware/i2c.h"
#include <stdbool.h>
#include <stdint.h>

#define SERIAL_INIT_DELAY_MS 1000 // adjust as needed to mitigate garbage characters after serial interface is started
#define EXTRA_DELAY_MS 1000 // for the optional extra delay
#define TMP117_I2C_SDA_PIN PICO_DEFAULT_I2C_SDA_PIN // set to a different SDA pin as needed
#define TMP117_I2C_SCL_PIN PICO_DEFAULT_I2C_SCL_PIN // set to a different SCL pin as needed
#define TMP117_OFFSET_VALUE 0.0f  // temperature offset in degrees C, set as desired

uint32_t get_conversion_delay(void);

int main(void) {
    // change if board is jumpered to a different address than 0x48
    // tmp117_set_address(0x49);

    // initialize chosen serial interface
    stdio_init_all();
    // a little delay to ensure serial line stability
    sleep_ms(SERIAL_INIT_DELAY_MS);
    
    // Selects I2C instance (i2c_default is set as default in the tmp117.c)
    //tmp117_set_instance(i2c1); // change to i2c1 as needed

    // initialize I2C
    i2c_init(i2c_instance, 400 * 1000); // TMP117 400 kHz max.
    // configure the GPIO pins for I2C
    gpio_set_function(TMP117_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(TMP117_I2C_SCL_PIN, GPIO_FUNC_I2C);

    // TMP117 software reset; loads EEPROM Power On Reset values
    soft_reset();

    // set_temp_offset(TMP117_OFFSET_VALUE); // can be useful to demonstrate negative temperatures

    // set continuous conversion mode if EEPROM has been set to shutdown mode
    uint8_t conversion_mode = get_conversion_mode();
    if (conversion_mode != 0)
        set_continuous_conversion_mode();

    // put a space in the serial output before loop begins
    printf("\n");
    
    // test different conversion cycle times, see Table 7-7 of the datasheet
    set_conversion_cycle(CONV_4_S);     // set desired cycle time
    set_averaging_mode(AVG_64);         // set desired averaging mode

    // get conversion delay based on conversion cycle and averaging bits
    uint32_t delay_ms = get_conversion_delay();

    // optionally display the delay to the serial once
    printf("Conversion Cycle Time: %d ms\n",delay_ms);

    // a conversion cycle time less than 1 second contributes to the SHE - Self Heating Effect, see datasheet
    while (true) {

        do {
            sleep_ms(delay_ms);
        } while (!data_ready()); // check if the data ready flag is high

        // 1) typecast temp_result register to integer, converting from two's complement
        // 2) Multiply by 100 to scale the temperature (i.e. 2 decimal places)
        // 3) Shift right by 7 to account for the TMP117's 1/128 resolution (Q7 format)
        int temp = read_temp_raw() * 100 >> 7;

        // Display the temperature in degrees Celsius, formatted to show two decimal places.
        // printf("Temperature: %d.%02d °C\n", temp / 100, (temp < 0 ? -temp : temp) % 100;

        // Or, Display the temperature in degrees Celsius, and degrees Fahrenheit
        float temp_float = temp / 100.0;
        printf("Temperature: %d.%02d °C \t%.2f °F\n", temp / 100, (temp < 0 ? -temp : temp) % 100, calc_temp_fahrenheit(temp_float));

        // floating point functions are also available for converting temp_result to Cesius or Fahrenheit directly
        // printf("\nTemperature: %.2f °C\t%.2f °F", read_temp_celsius(), read_temp_fahrenheit());

        // sleep_ms(EXTRA_DELAY_MS); // optional extra delay when cycle time is too fast, avoid < 1s due to SHE
    }

    return 0;
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