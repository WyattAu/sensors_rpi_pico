/**
 * Copyright (c) 2024 breaker
 * Based on code from:
 * - Raspberry Pi Pico SDK (c) 2020 Raspberry Pi (Trading) Ltd. (BSD-3-Clause License)
 * - SparkFun TMP117 Arduino Library (c) 2016 SparkFun Electronics (originally MIT, relicensed BSD-3-Clause)
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Original SparkFun TMP117 Arduino Library file: Example3_SetOffsetTemperatureValue.ino

/*
 * set_offset.c
 *
 * Example code for setting the TMP117 temperature offset register.
 *
 * Test board: SparkFun High Precision Temperature Sensor - TMP117 (Qwiic)
 *
 * In this example the temp_offset register setting is volatile and will reset on power cycle. 
 * See the EEPROM example for setting a new Power On Reset value for writable registers.
 *
 * This code displays the hex value of the temperature offset register, use this information
 * to test decimal values from Table 7-1 in the TMP117 datasheet to verify correct conversion.
 *   e.g. -25, –0.1250, –0.0078125, 0.0078125, 0.1250, 25, etc.
 *
 * See TMP117 datasheet, section 7.6.9, Temperature Offset Register (address = 0x07).
 *
 */

#include "tmp117_registers.h"
#include "tmp117.h"
#include "pico/stdlib.h"
#include "pico/printf.h"
#include "hardware/i2c.h"
#include "hardware/watchdog.h"
#include <stdbool.h>
#include <stdint.h>

#define SERIAL_INIT_DELAY_MS 1000 // change to suit serial interface used
#define TMP117_I2C_SDA_PIN PICO_DEFAULT_I2C_SDA_PIN // set to a different SDA pin as needed
#define TMP117_I2C_SCL_PIN PICO_DEFAULT_I2C_SCL_PIN // set to a different SCL pin as needed

int main(void)
{
  // init selected serial interface
  stdio_init_all();
  // adjust delay to mitigate garbage on the serial line
  sleep_ms(SERIAL_INIT_DELAY_MS);
  // initialize I2C0 (default)
  i2c_init(i2c_instance, 400 * 1000); // 400 kHz
  // set I2C pins - configure the GPIO pins for I2C
  gpio_set_function(TMP117_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(TMP117_I2C_SCL_PIN, GPIO_FUNC_I2C);

  printf("\n");

  soft_reset(); // TMP117 software reset; loads EEPROM Power On Reset values
  set_shutdown_mode(); // go into shutdown mode to stop continuous conversions

  while (true) {
    // enable one-shot mode
    set_oneshot_mode();

    // wait the max averaging time of 1 second, check the data_ready flag
    do {
      sleep_ms(1000);
    } while(!data_ready());

    // print temp_offset register hex value, floating point value, and read temperature
    printf("\nTemp_Offset register in hex: \t0x%04X", read_register(TMP117_TEMP_OFFSET));
    printf("\nTemperature Offset: \t\t%.7f °C", get_temp_offset());
    printf("\nTemperature with Offset: \t%.2f °C\n", read_temp_celsius());

    float temp_offset = 0;
    bool valid_input = 0;

    printf("\nEnter new temperature offset (-256 to +255.9921875 °C): ");
        
    // Read the input from the serial port
    if ((valid_input = (scanf("%f", &temp_offset)) == 0)) {
        break;
    }
    else if (temp_offset > 255.9921875 || temp_offset < -256.0) {
        printf("\nPlease enter a number within the range of -256 to +255.9921875 °C\n");
        continue;
        }
    else {
      set_temp_offset(temp_offset); // Write to the temperature offset register
    }
  }

  // reboot on bad input
  printf("\nInvalid characters, reboot...");
  sleep_ms(100);
  watchdog_reboot(0, 0, 0);

  return 0; // it should never reach this
}
