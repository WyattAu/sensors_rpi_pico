// eeprom.c
/**
 * Copyright (c) 2024 breaker
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* Example to program TMP117 EEPROM for the Temperature Offset register 07h
 * Try values from table 7-1 of the datasheet to verify correct operation,
 *  e.g. -25, –0.1250, –0.0078125, 0.0078125, 0.1250, 25, etc.
 */

/* NOTE: This example will program new Power On Reset values into the TMP117 EEPROM
 * Use the read registers or NIST-ID example to display the factory set values for EEPROMs 1-3
 * Take those values and note them, change the value of TMP117_EEPROM1_VALUE below to match
 */

// WARNING: This example will overwrite the NIST value of EEPROM1

#include "tmp117.h"
#include "tmp117_registers.h"
#include "pico/stdlib.h"
#include "pico/printf.h"
#include "hardware/i2c.h"
#include <stdbool.h>
#include <stdint.h>

// usually the I2C pins and default instance are specified in a board header
#define TMP117_I2C_SDA_PIN PICO_DEFAULT_I2C_SDA_PIN // set to a different SDA pin as needed
#define TMP117_I2C_SCL_PIN PICO_DEFAULT_I2C_SCL_PIN // set to a different SCL pin as needed
#define SERIAL_INIT_DELAY_MS 1000 // change to suit serial interface used

// test programming offset and/or temperature high and low limits
#define TMP117_OFFSET_VALUE 0.0f  // temperature offset in degrees C set by user
#define TMP117_HIGH_LIMIT_VALUE 0.0f // temperature high limit in degrees C set by user
#define TMP117_LOW_LIMIT_VALUE 0.0f // temperature low limit in degrees C set by user

// for testing the programming of the general purpose EEPROMs (NIST-ID or scratchpad)
#define TMP117_EEPROM1_VALUE 0x1D65 // input your default
//#define TMP117_EEPROM1_VALUE 0x0000 // test a different value
#define TMP117_EEPROM2_VALUE 0x0F3E // input your default
//#define TMP117_EEPROM2_VALUE 0x0000
#define TMP117_EEPROM3_VALUE 0x0F3E // input your default
//#define TMP117_EEPROM3_VALUE 0x0000
#define example_alert example_alert_mode(ALERT_MODE);   // program to ALERT_MODE
//#define example_alert example_alert_mode(THERM_MODE); // program to THERM_MODE

void example_temperature_offset(float desired_offset);
void example_alert_mode(uint8_t mode);
void example_high_limit(float high_limit);
void example_low_limit(float low_limit);
void example_eeprom1(uint16_t value);

// Program new values if different than current, report to serial
int main() {
    stdio_init_all();
    sleep_ms(SERIAL_INIT_DELAY_MS); // wait for noise on serial to die down
    i2c_init(i2c_instance, 400 * 1000); // 400 kHz
    gpio_set_function(TMP117_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(TMP117_I2C_SCL_PIN, GPIO_FUNC_I2C);
    soft_reset(); // soft reset TMP117

    printf("\n\nWelcome to TMP117 EEPROM programming!");

    example_temperature_offset(TMP117_OFFSET_VALUE);
    example_alert;
    example_high_limit(TMP117_HIGH_LIMIT_VALUE);
    example_low_limit(TMP117_LOW_LIMIT_VALUE);
    example_eeprom1(TMP117_EEPROM1_VALUE);

    printf("\n");
    
    while(1) {
        tight_loop_contents();
    };

}

void example_temperature_offset(float desired_offset) {
    float current_offset = get_temp_offset();
    printf("\nCurrent Temperature Offset: %.7f °C (Temp_Offset 0x%04X)", 
           current_offset, read_register(TMP117_TEMP_OFFSET));

    if (current_offset != desired_offset) {
        printf("\nSetting offset in EEPROM to: %.7f °C", desired_offset);
        if (program_offset(desired_offset))
            printf("\nOffset set OK!");
        else
            printf("\nOffset set NG!");
        printf("\nCurrent Temperature Offset: %.7f °C (Temp_Offset 0x%04X)", 
               get_temp_offset(), read_register(TMP117_TEMP_OFFSET));
    }
}

void example_alert_mode(uint8_t mode) {
    uint8_t current_mode = get_thermalert_mode();

    printf("\nCurrent alert mode: ");
        if (current_mode)
            printf("Therm mode");
        else
            printf("Alert mode");

    if (current_mode != mode) {
        if (program_thermalert_mode(mode)) {
            printf("\nNew alert mode set to: ");
                if (mode)
                    printf("Therm mode");
                else
                    printf("Alert mode");
        }
        else
            printf("\nError changing alert mode!");
    }
}

void example_high_limit(float high_limit) {
    float current_high_limit = get_high_limit();
    printf("\nCurrent High Limit: %.7f °C (THigh_Limit 0x%04X)", 
           current_high_limit, read_register(TMP117_T_HIGH_LIMIT));

    if (current_high_limit != high_limit) {
        printf("\nSetting High Limit in EEPROM to: %.7f °C", high_limit);
        if (program_high_limit(high_limit))
            printf("\nHigh Limit set OK!");
        else
            printf("\nHigh Limit set NG!");
        printf("\nCurrent High Limit: %.7f °C (THigh_Limit 0x%04X)", 
               get_high_limit(), read_register(TMP117_T_HIGH_LIMIT));
    }
}

void example_low_limit(float low_limit) {
    float current_low_limit = get_low_limit();
    printf("\nCurrent Low Limit: %.7f °C (TLow_Limit 0x%04X)", 
           current_low_limit, read_register(TMP117_T_LOW_LIMIT));

    if (current_low_limit != low_limit) {
        printf("\nSetting Low Limit in EEPROM to: %.7f °C", low_limit);
        if (program_low_limit(low_limit))
            printf("\nLow Limit set OK!");
        else
            printf("\nLow Limit set NG!");
        printf("\nCurrent Low Limit: %.7f °C (TLow_Limit 0x%04X)", 
               get_low_limit(), read_register(TMP117_T_LOW_LIMIT));
    }
}

// EEPROM1 programming
void example_eeprom1(uint16_t value) {
    uint16_t current_eeprom1 = read_register(TMP117_EEPROM1);
    printf("\nCurrent EEPROM1: 0x%04X", current_eeprom1);

    if (current_eeprom1 != TMP117_EEPROM1_VALUE) {
        printf("\nSetting value in EEPROM1 to: 0x%04X",TMP117_EEPROM1_VALUE);

        if (program_eeprom1(TMP117_EEPROM1_VALUE))
            printf("\nEEPROM1 set OK!");
        else
            printf("\nEEPROM1 set NG!");

        printf("\nCurrent EEPROM1: 0x%04X", read_register(TMP117_EEPROM1));
    }
}
