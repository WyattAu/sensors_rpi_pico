#include "tmp117.h"
#include "tmp117_registers.h"
#include "pico/stdlib.h"
#include "pico/printf.h"
#include "hardware/i2c.h"

// Check if TMP117 is at the specified address and has correct device ID
void check_status(uint frequency) {
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
void check_i2c(uint frequency) {
    if (frequency == 0) {
        printf("I2C has no clock.\n");
        while(1) {
            tight_loop_contents();  // Halt execution if I2C frequency is zero
        }
    }
}