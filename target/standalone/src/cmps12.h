#ifndef CMPS12_H
#define CMPS12_H

#include <stdint.h>
#include <stdbool.h>

#ifndef HOST_TESTING
#include "hardware/i2c.h"
#else
// Mock types for host testing
typedef struct i2c_inst i2c_inst_t;
struct i2c_inst {};
#endif


// I2C Address and Register
#define CMPS12_ADDRESS 0x60
#define ANGLE_8_REG 1

// Structure to hold compass data
typedef struct {
    i2c_inst_t *i2c;
    uint8_t angle8;
    uint16_t angle16;
    int8_t pitch;
    int8_t roll;
} cmps12_t;

// Function declarations
#ifdef __cplusplus
extern "C" {
#endif

bool cmps12_init(cmps12_t *compass, i2c_inst_t *i2c);
bool cmps12_read(cmps12_t *compass);
const char* cmps12_get_cardinal_direction(uint16_t angle);

#ifdef __cplusplus
}
#endif

#endif