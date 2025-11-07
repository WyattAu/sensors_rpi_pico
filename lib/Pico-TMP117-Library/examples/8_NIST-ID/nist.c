// nist.c
/**
 * Copyright (c) 2024 breaker
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*The TMP117 datasheet does not indicate the endianess of the NIST ID,
however, registers 05h, 06h and 08h contain the NIST data.*/

#include "tmp117.h"
#include "tmp117_registers.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/printf.h"
#include "pico/stdio.h"
#include <stdbool.h>
#include <stdint.h>

#define SERIAL_INIT_DELAY_MS 1000 // adjust as needed to mitigate garbage characters after serial interface is started
#define TMP117_I2C_SDA_PIN PICO_DEFAULT_I2C_SDA_PIN // set to a different SDA pin as needed
#define TMP117_I2C_SCL_PIN PICO_DEFAULT_I2C_SCL_PIN // set to a different SCL pin as needed

int main() {
    stdio_init_all();
    sleep_ms(SERIAL_INIT_DELAY_MS); // wait for noise on serial to die down

    // Selects I2C instance (i2c0 is set as default in the tmp117.c)
    //tmp117_set_instance(i2c1); // change to i2c1 as needed
    
    i2c_init(i2c_instance, 400 * 1000); // 400 kHz
    gpio_set_function(TMP117_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(TMP117_I2C_SCL_PIN, GPIO_FUNC_I2C);
    soft_reset(); // TMP117 software reset; loads EEPROM Power On Reset values

    // Is it actually in the order 3,2,1? I am not sure.
    printf("\n\nWelcome to TMP117 NIST ID!");
    uint16_t eep1 = read_register(TMP117_EEPROM1);
    uint16_t eep2 = read_register(TMP117_EEPROM2);
    uint16_t eep3 = read_register(TMP117_EEPROM3);
    printf("\nEEPROM NIST ID 0x%04X%04X%04X",(uint16_t)eep1,(uint16_t)eep2,(uint16_t)eep3);
}