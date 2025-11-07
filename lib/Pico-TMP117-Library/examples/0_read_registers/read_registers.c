// read_registers.c
/**
 * Copyright (c) 2024 breaker
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// read and print the values of all registers and flags
// perhaps useful to determine the current Power On Reset values, or to get an overview of the sensor

#include "tmp117.h"
#include "tmp117_registers.h"
#include "pico/stdlib.h"
#include "pico/printf.h"
#include "hardware/i2c.h"
#include <stdbool.h>
#include <stdint.h>

#define SERIAL_INIT_DELAY_MS 1000 // adjust as needed to mitigate garbage characters after serial interface is started
#define TMP117_I2C_SDA_PIN PICO_DEFAULT_I2C_SDA_PIN // set to a different SDA pin as needed
#define TMP117_I2C_SCL_PIN PICO_DEFAULT_I2C_SCL_PIN // set to a different SCL pin as needed

void check_i2c(void);
void check_status(void);
void extract_flags(uint16_t configuration);

// variable for checking I2C frequency
uint frequency = 0;
// variables for all of the bits of the configuration register
uint8_t high_alert_flag = 2;
uint8_t low_alert_flag = 2;
uint8_t data_ready_flag = 1;
uint8_t eeprom_busy_flag = 2;
uint8_t conversion_mode = 4;
uint8_t conversion_cycle_bit = 8;
uint8_t conversion_average = 4;
uint8_t therm_alert_mode = 2;
uint8_t polarity_alert_pin = 2;
uint8_t alert_pin_select = 2;
uint8_t software_reset = 2;
uint8_t not_used = 2;

int main(void) {
    // initialize chosen interface
    stdio_init_all();
    // a little delay to ensure serial line stability
    sleep_ms(SERIAL_INIT_DELAY_MS);

    // Selects I2C instance (i2c0 is set as default in the tmp117.c)
    //tmp117_set_instance(i2c1); // change to i2c1 as needed
    
    // initialize I2C (default i2c0) and initialize variable with I2C frequency
    frequency = i2c_init(i2c_instance, 400 * 1000); // TMP117 400 kHz max.
    // configure the GPIO pins for I2C
    gpio_set_function(TMP117_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(TMP117_I2C_SCL_PIN, GPIO_FUNC_I2C);

    // call a function to check and report if i2c is running
    check_i2c();

    // uncomment below to set I2C address other than 0x48 (e.g., 0x49)
    //tmp117_set_address(0x49);

    printf("\n----------------------------------------------------\n");

    // check if TMP117 is on the I2C bus at the address specified
    check_status();

    soft_reset(); // TMP117 software reset; loads EEPROM Power On Reset values

    // put configuration register into a variable for bit extraction
    uint16_t config = read_register(TMP117_CONFIGURATION);

    // display the hex values of all registers. 01h-08h are R/W 00h and 0Fh read only
    printf("----------------------------------------------------\n");
    printf(("## register \thex reading\n"));
    printf("00 Temperature:      0x%04X\n",(uint16_t)read_register(TMP117_TEMP_RESULT));
    printf("01 Configuration:    0x%04X\n",(uint16_t)config);
    printf("02 THigh_Limit:      0x%04X\n",(uint16_t)read_register(TMP117_T_HIGH_LIMIT));
    printf("03 TLow_Limit:       0x%04X\n",(uint16_t)read_register(TMP117_T_LOW_LIMIT));
    printf("04 EEPROM_UL:        0x%04x\n",(uint16_t)read_register(TMP117_EEPROM_UL));
    printf("05 EEPROM1:          0x%04X\n",(uint16_t)read_register(TMP117_EEPROM1));
    printf("06 EEPROM2:          0x%04X\n",(uint16_t)read_register(TMP117_EEPROM2));
    printf("07 Temp_Offset:      0x%04X\n",(uint16_t)read_register(TMP117_TEMP_OFFSET));
    printf("08 EEPROM3:          0x%04X\n",(uint16_t)read_register(TMP117_EEPROM3));
    printf("0F Device ID:        0x%04X\n\n",(uint16_t)read_register(TMP117_DEVICE_ID));
    // extract bits of configuration register, print it all
    extract_flags(config);
    printf("Configuration register bits;\n");
    printf("----------------------------------------------------\n");
    printf("15 HIGH_Alert flag  = %d\n",high_alert_flag);
    printf("14 LOW_Alert flag   = %d\n",low_alert_flag);
    printf("13 Data_Ready flag  = %d\n",data_ready_flag);
    printf("12 EEPROM_Busy flag = %d\n",eeprom_busy_flag);
    printf("11:10 MOD[1:0] conversion mode = 0x%X\n",(uint8_t)conversion_mode);
    printf("\t00: Continuous conversion (CC)\n\t01: Shutdown (SD)\n\t11: One-shot conversion (OS)\n");
    printf("9:7 CONV[2:0] Conversion cycle bit = 0x%X (see table 7-7 of datasheet)\n",(uint8_t)conversion_cycle_bit);
    printf("6:5 AVG[1:0] Conversion averaging mode = 0x%X\n",(uint8_t)conversion_average);
    printf("\t00: No averaging\n\t01: 8 Averaged conversions\n\t10: 32 averaged conversions\n\t11: 64 averaged conversions\n");
    printf("4 T/nA Therm/alert mode select = %d\n\t1: Therm mode\n\t0: Alert mode\n",(uint8_t)therm_alert_mode);
    printf("3 POL Alert pin polarity bit = %d\n",(uint8_t)polarity_alert_pin);
    printf("\t1: Active High\n\t0: Active Low.\n");
    printf("2 Alert pin bit select = %d\n",(uint8_t)alert_pin_select);
    printf("\t1: ALERT pin reflects the status of the data ready flag\n");
    printf("\t0: ALERT pin reflects the status of the alert flags\n");
    printf("1 Soft_Reset Software reset bit (will always read back 0) = %d\n",software_reset);
    printf("0 Not used (reset 0) = %d\n",not_used);

while(1) {
    tight_loop_contents();
}

return 0;
}

// This method is less efficient than using bitwise operations on the value of the configuration register
void extract_flags(uint16_t config) {
    CONFIGURATION_REG configuration_register = { .CONFIGURATION_COMBINED = config }; // initialize with config

    high_alert_flag = configuration_register.CONFIGURATION_FIELDS.HIGH_ALERT;       // Bit 15
    low_alert_flag = configuration_register.CONFIGURATION_FIELDS.LOW_ALERT;         // Bit 14
    data_ready_flag = configuration_register.CONFIGURATION_FIELDS.DATA_READY;       // Bit 13
    eeprom_busy_flag = configuration_register.CONFIGURATION_FIELDS.EEPROM_BUSY;     // Bit 12
    conversion_mode = configuration_register.CONFIGURATION_FIELDS.MOD;              // Bits 10-11
    conversion_cycle_bit = configuration_register.CONFIGURATION_FIELDS.CONV;        // Bits 7-9
    conversion_average = configuration_register.CONFIGURATION_FIELDS.AVG;           // Bits 5-6
    therm_alert_mode  = configuration_register.CONFIGURATION_FIELDS.T_NA;           // Bit 4
    polarity_alert_pin = configuration_register.CONFIGURATION_FIELDS.POL;           // Bit 3
    alert_pin_select = configuration_register.CONFIGURATION_FIELDS.DR_ALERT;        // Bit 2
    software_reset = configuration_register.CONFIGURATION_FIELDS.TMP_SOFT_RESET;    // Bit 1
    not_used = configuration_register.CONFIGURATION_FIELDS.EMPTY;                   // Bit 0
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

// check if I2C is running
void check_i2c(void) {
    if (frequency == 0) {
        printf("I2C has no clock.\n");
        while(1) {
            tight_loop_contents();  // Halt execution if I2C frequency is zero
        };
    }
}