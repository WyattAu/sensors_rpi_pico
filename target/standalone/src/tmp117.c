#include "tmp117.h"
#include "tmp117_registers.h"
#include <stdint.h>

#ifndef HOST_TESTING
#include "pico/stdlib.h"
#include "pico/printf.h"
#include "hardware/i2c.h"

// Mock Pico SDK error codes
#define PICO_ERROR_TIMEOUT -3
#define PICO_ERROR_GENERIC -1

// Mock timeout
#define SMBUS_TIMEOUT_US 100000

// Mock TMP117 codes
#define TMP117_OK 0
#define TMP117_ID_NOT_FOUND -2
#endif

// Mock functions for testing
uint8_t tmp117_get_address(void) {
    return 0x48;  // Default TMP117 address
}

int begin(void) {
    return TMP117_OK;  // Mock success
}

// Check if TMP117 is at the specified address and has correct device ID
void check_status(unsigned int frequency) {
    uint8_t address = tmp117_get_address();
    int status = begin();

    switch (status) {
        case TMP117_OK:
            printf("\nTMP117 found at address 0x%02X, I2C frequency %dkHz\n", 
                   address, frequency / 1000);
            break;

        case PICO_ERROR_TIMEOUT:
            printf("\nI2C timeout reached after %u microseconds\n", SMBUS_TIMEOUT_US);
            while (1) {
                tight_loop_contents();  // Halt execution if timeout occurs
            }
            break;

        case PICO_ERROR_GENERIC:
            printf("\nNo I2C device found at address 0x%02X\n", address);
            while (1) {
                tight_loop_contents();  // Halt execution if no device found
            }
            break;

        case TMP117_ID_NOT_FOUND:
            printf("\nNon-TMP117 device found at address 0x%02X\n", address);
            while (1) {
                tight_loop_contents();  // Halt execution if a wrong device is found
            }
            break;

        default:
            printf("\nUnknown error during TMP117 initialization\n");
            while (1) {
                tight_loop_contents();  // Halt execution for unexpected errors
            }
        }
        
}

// Check if I2C is running on Pico
void check_i2c(unsigned int frequency) {
    if (frequency == 0) {
        printf("I2C has no clock.\n");
        while(1) {
            tight_loop_contents();  // Halt execution if I2C frequency is zero
        }
    }
}

// Mock implementations for missing functions
void soft_reset(void) {
    // Mock soft reset
    printf("TMP117 soft reset performed.\n");
}

bool data_ready(void) {
    // Mock data ready check
    return true;
}

int read_temp_raw(void) {
    // Mock temperature reading
    return 2500;  // 25.00 degrees in TMP117 format
}

float read_temp_celsius(void) {
    // Mock Celsius reading
    return 25.0f;
}

float read_temp_fahrenheit(void) {
    // Mock Fahrenheit reading
    return 77.0f;
}