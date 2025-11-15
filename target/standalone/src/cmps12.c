#include "cmps12.h"

#ifndef HOST_TESTING
#include "pico/stdlib.h"
#endif

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Initialize the CMPS12 compass
bool cmps12_init(cmps12_t *compass, i2c_inst_t *i2c) {
    compass->i2c = i2c;
    compass->angle8 = 0;
    compass->angle16 = 0;
    compass->pitch = 0;
    compass->roll = 0;
    
    // Test if device is present
    uint8_t test_byte;
    int result = i2c_read_blocking(i2c, CMPS12_ADDRESS, &test_byte, 1, false);
    
    return (result >= 0);
}

// Read all data from CMPS12
bool cmps12_read(cmps12_t *compass) {
    uint8_t data[5];
    uint8_t reg = ANGLE_8_REG;
    
    // Write register address
    int result = i2c_write_blocking(compass->i2c, CMPS12_ADDRESS, &reg, 1, true);
    if (result < 0) {
        return false;
    }
    
    // Read 5 bytes: angle8, high_byte, low_byte, pitch, roll
    result = i2c_read_blocking(compass->i2c, CMPS12_ADDRESS, data, 5, false);
    if (result < 0) {
        return false;
    }
    
    // Parse the data
    compass->angle8 = data[0];
    uint8_t high_byte = data[1];
    uint8_t low_byte = data[2];
    compass->pitch = (int8_t)data[3];
    compass->roll = (int8_t)data[4];
    
    // Calculate 16-bit angle
    compass->angle16 = (high_byte << 8) | low_byte;
    
    return true;
}

// Get cardinal direction from angle (0-359 degrees)
const char* cmps12_get_cardinal_direction(uint16_t angle) {
    static const char* directions[] = {
        "N", "NNE", "NE", "ENE", 
        "E", "ESE", "SE", "SSE",
        "S", "SSW", "SW", "WSW", 
        "W", "WNW", "NW", "NNW"
    };
    
    // Divide by 22.5 and round to nearest direction
    int val = (int)((angle / 22.5) + 0.5);
    
    // Wrap around using modulus 16
    return directions[val % 16];
}
