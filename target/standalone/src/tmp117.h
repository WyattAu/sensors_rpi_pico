#ifndef TMP117_H
#define TMP117_H

#include <stdint.h>
#include <stdbool.h>

// Define types for host testing
#ifdef HOST_TESTING
typedef unsigned int uint;
#endif

// Function declarations
#ifdef __cplusplus
extern "C" {
#endif


void check_i2c(unsigned int frequency);
void check_status(unsigned int frequency);
void soft_reset(void);
bool data_ready(void);
int read_temp_raw(void);
float read_temp_celsius(void);
float read_temp_fahrenheit(void);

#ifdef __cplusplus
}
#endif

#endif