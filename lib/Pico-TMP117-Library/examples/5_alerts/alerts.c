// /tmp117/examples/alerts.c
/**
 * Copyright (c) 2024 breaker
 * Based on code from:
 * - Raspberry Pi Pico SDK (c) 2020 Raspberry Pi (Trading) Ltd. (BSD-3-Clause License)
 * - SparkFun TMP117 Arduino Library (c) 2016 SparkFun Electronics (originally MIT, relicensed BSD-3-Clause)
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Original SparkFun TMP117 Arduino Library file: Example2_AlertStatuses.ino

/**
 * @file alerts.c
 * @brief Example program for configuring and reading from the TMP117 sensor on the Pico.
 * 
 * After each temperature conversion, the TMP117 compares the result to the values stored
 * in the high and low limit registers and sets or clears the corresponding status flags.
 *
 * This program demonstrates how to set temperature limits, configure alert modes, and
 * read the high temperature, low temperature, and data ready flags.
 *
 * In therm mode, the high alert flag is not cleared until the temperature goes below the low alert limit setting.
 * In therm mode, the low alert flag is disabled and always reads 0.
 * In Alert mode, the high alert flag is cleared when then temperature goes below the high alert limit setting
 * (or when the temp_result or configuration register is read).
 *
 * TMP117 datasheet section references (I recommend reading all of section 7):
 *  - 7.3.3 Temperature Result and Limits
 *  - 7.4.3 One-Shot Mode (OS)
 *  - 7.4.4 Therm and Alert Modes
 *  - 7.6.4 High Limit Register (address = 02h) [Factory default reset = 6000h]
 *  - 7.6.5 Low Limit Register (address = 03h) [Factory default reset = 8000h]
 *
 * @note 
 * - Data in the offset, temperature, high limit, and low limit registers:
 *   - Format: 16-bit two's complement
 *   - Resolution: 7.8125 m°C
 * - Reading the configuration register clears the High Alert and Low Alert flags
 * - Reading the configuration or temperature result registers clears the Data Ready flag.
 * - Use `set_alert_pin(1)` to monitor the Data_Ready flag on the ALERT pin.
 * - The MOD1 bit cannot be stored in EEPROM.
 * - The device can only be programmed to start up in shutdown mode or continuous conversion mode.
 */

#include "tmp117.h"
#include "tmp117_registers.h"
#include "pico/stdlib.h"
#include "pico/printf.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include <stdbool.h>
#include <stdint.h>

#define SERIAL_INIT_DELAY_MS 1000 // change to suit serial interface used
#define TMP117_I2C_SDA_PIN PICO_DEFAULT_I2C_SDA_PIN // set to a different SDA pin as needed
#define TMP117_I2C_SCL_PIN PICO_DEFAULT_I2C_SCL_PIN // set to a different SCL pin as needed
#define EXTRA_DELAY_MS 1000 // for the optional extra delay

uint32_t get_conversion_delay(void);

int main() {
  // init selected serial interface
  stdio_init_all();
  sleep_ms(SERIAL_INIT_DELAY_MS);

// ensure board file has I2C pins
#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
#warning alerts example requires a board with I2C pins
    puts("Default I2C pins were not defined");
#else

  // initialize I2C0 (default)
  i2c_init(i2c_instance, 400 * 1000); // 400 kHz
  // set I2C pins - configure the GPIO pins for I2C
  gpio_set_function(TMP117_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(TMP117_I2C_SCL_PIN, GPIO_FUNC_I2C);

  soft_reset(); // TMP117 software reset; loads EEPROM Power On Reset values

  // set continuous conversion mode if EEPROM has been set to shutdown mode
    uint8_t conversion_mode = get_conversion_mode();
    if (conversion_mode != CC)
        set_continuous_conversion_mode();

  // See Table 7-7. Conversion Cycle Time in Continuous Conversion Mode of datasheet
  // choices; AVG_NONE, AVG_8, AVG_32, AVG_64
  set_averaging_mode(AVG_8);
  // choices; CONV_15_5_MS, CONV_125_MS, CONV_250_MS, CONV_500_MS, CONV_1000_MS, CONV_4000_MS, CONV_8000_MS, CONV_16000_MS
  set_conversion_cycle(CONV_1_S);
  // call a function to get the conversion cycle time from an array, initialize into a variable
  uint32_t delay_ms = get_conversion_delay();
  /*Note: Set the high and low limits between -256°C and 255.9921875°C.
    For a test use your breath or finger to warm the sensor.*/
  set_high_limit(21.16); // set high limit (THigh_Limit register 02h)
  set_low_limit(18.7); // set low limit (TLow_Limit register 03h)

/* Test this program in both therm and alert modes to observe the differences.
     In therm mode, only High Alert and No Alert states are displayed.
     The High Alert flag remains active until the temperature falls below the low limit,
     effectively creating a hysteresis effect.
     Note that in therm mode, the low alert flag is always zero. */

  // Use THERM_MODE or ALERT_MODE to set the Alert mode
  //set_thermalert_mode(THERM_MODE);
  set_thermalert_mode(ALERT_MODE);

  // Get the high and low temperature limit values in degrees Celsius
  printf("\n--------------------------------------------------");
  // Get High Temperature Limit 
  printf("\nHigh Temperature Limit: %.2f °C",get_high_limit());
  // Get Low Temperature Limit
  printf("\nLow Temperature Limit:  %.2f °C",get_low_limit());
  // Get Alert Function Mode Bit from configuration register
  bool mode = get_thermalert_mode();
  printf("\nAlert Function Mode: ");
  if (mode == 1)
    printf("%d Therm", mode);
  else
    printf("%d Alert", mode);
  printf("\ndelay in ms %u", delay_ms + EXTRA_DELAY_MS);
  printf("\n--------------------------------------------------");

while (1) {
    // use the below to test one-shot mode
    //set_oneshot_mode();
    if (get_conversion_mode() == OS)
      delay_ms = 1000;
    
    // see tmp117.h for struct TMP117_flags_t (holds bool of data_ready, high_alert, low_alert)
    tmp117_flags_t flags; // Declare a struct to hold the flags

    // delay, get values of the status flags, loop until the data_ready flag is high
    do {
      sleep_ms(delay_ms); // 1 second is the longest conversion delay possible with one-shot mode
      sleep_ms(EXTRA_DELAY_MS); // optional extra delay (avoid a total delay < 1s due to the Self Heating Effect)
      flags = get_status_flags(); // Get the latest flag status (thus clearing them)
    } while (!flags.data_ready); // If flags.data_ready is true, data is ready

    // Read the temperature result register and use floating-point math to convert to Celsius
    float temp_celsius = read_temp_celsius();
    // print Celsius temperature to serial, calculate and print Fahrenheit temperature to serial
    printf("\nTemperature: %.2f °C\t%.2f °F\t", temp_celsius, calc_temp_fahrenheit(temp_celsius));

    // display which status flag is active, if any
    if (flags.low_alert) {
      printf("Low Alert");
    }
    else if (flags.high_alert) {
      printf("High Alert");
    }
    else {
      printf("No Alert");
    }

}

  return 0;

  #endif
} // end main

// Determine conversion delay based on the conversion cycle time and averaging mode bits
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