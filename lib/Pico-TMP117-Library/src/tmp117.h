/**
 * Copyright (c) 2024 breaker
 * Based on code from:
 * - Raspberry Pi Pico SDK (c) 2020 Raspberry Pi (Trading) Ltd. (BSD-3-Clause License)
 * - SparkFun TMP117 Arduino Library (c) 2016 SparkFun Electronics (originally MIT, relicensed BSD-3-Clause)
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Original SparkFun TMP117 Arduino Library file: SparkFunTMP117.h

// This version is for use as an INTERFACE library to the Raspberry Pi Pico SDK

#ifndef TMP117_H
#define TMP117_H

#ifndef __unused
#define __unused __attribute__((unused))
#endif

#include "hardware/i2c.h"

#define TMP117_DEFAULT_ADDRESS 0x48     // Default I2C address for SparkFun TMP117 Qwiic
#define DEVICE_ID_VALUE 0x0117			// Value found in the device ID register
#define TMP117_RESOLUTION 0.0078125f	// Resolution of the device (1/128)
#define TMP117_RESET_DELAY_MS 2         // power-on reset or soft reset delay (1.5 to 2ms)
#define SMBUS_TIMEOUT_US 35000          // timeout for SMBus I2C read in microseconds
#define EEPROM_UNLOCK_VALUE 0x8000      // value to write to the EEPROM unlock register to unlock it

// declaration of external variable for setting I2C instance
extern i2c_inst_t *i2c_instance;

// struct for high_alert, low_alert, and data_ready status flags
typedef struct {
    bool high_alert;
    bool low_alert;
    bool data_ready;
} tmp117_flags_t;

// conversion modes; CC = 0b00, SD = 0b01, OS = 0b11
typedef enum {
	CC = 0b00,  // continuous conversion
	SD = 0b01,  // shutdown
	OS = 0b11,  // one-shot
} tmp117_conversion_mode_t;

// conversion cycle bit (base conversion cycle time)
typedef enum {
    CONV_15_5_MS = 0b000,   // 15.5 ms
    CONV_125_MS  = 0b001,   // 125 ms
    CONV_250_MS  = 0b010,   // 250 ms
    CONV_500_MS  = 0b011,   // 500 ms
    CONV_1_S = 0b100,       // 1 second
    CONV_4_S = 0b101,       // 4 seconds
    CONV_8_S = 0b110,       // 8 seconds
    CONV_16_S = 0b111,      // 16 seconds
} tmp117_conversion_cycle_t;

// conversion averaging modes;
// AVG_NONE = no averaging, AVG_8, AVG_32, AVG_64 (averaged conversions)
typedef enum {
    AVG_NONE = 0b00,    // No averaging
    AVG_8 = 0b01,       // 8 averaged conversions
    AVG_32 = 0b10,      // 32 averaged conversions
    AVG_64 = 0b11,      // 64 averaged conversions
} tmp117_avg_mode_t;

// THERM_MODE = 1, ALERT_MODE = 0
typedef enum {
    THERM_MODE = 1,
    ALERT_MODE = 0,
} tmp117_alert_mode_t;

// ALERT_PIN_ALERT_FLAGS = 0, ALERT_PIN_DATA_READY = 1
typedef enum {
    ALERT_PIN_DATA_READY = 1,   // ALERT pin reflects the status of the data ready flag
    ALERT_PIN_ALERT_FLAGS = 0,  // ALERT pin reflects the status of the alert flags
} tmp117_alert_pin_select_t;

// enum for TMP117 specific error codes
typedef enum {
    TMP117_OK = 0,                  // No error
    TMP117_ERROR_GENERIC = -22,     // Generic error
    TMP117_ID_NOT_FOUND = -23,      // Device ID not found
} tmp117_error_codes_t;

// This typdef was copied directly from the original SparkFun_TMP117.h
// See the TMP117 datasheet: section 7.6.3, and Figure 7-14. Configuration register
typedef union {
	struct
	{
		uint8_t EMPTY : 1;			// Empty bit in register
		uint8_t TMP_SOFT_RESET : 1; // Software reset bit
		uint8_t DR_ALERT : 1;		// ALERT pin select bit
		uint8_t POL : 1;			// ALERT pin polarity bit
		uint8_t T_NA : 1;			// Therm/alert mode select
		uint8_t AVG : 2;			// Conversion averaging modes
		uint8_t CONV : 3;			// Conversion cycle bit
		uint8_t MOD : 2;			// Set conversion mode
		uint8_t EEPROM_BUSY : 1;	// EEPROM busy flag
		uint8_t DATA_READY : 1;		// Data ready flag
		uint8_t LOW_ALERT : 1;		// Low Alert flag
		uint8_t HIGH_ALERT : 1;		// High Alert flag
	} CONFIGURATION_FIELDS;

	uint16_t CONFIGURATION_COMBINED;
} CONFIGURATION_REG;

// Function prototypes
int begin(void);
void tmp117_set_address(uint8_t address);
uint8_t tmp117_get_address(void);
void tmp117_set_instance(i2c_inst_t *instance);
uint16_t read_register(uint8_t reg);
void write_register(uint8_t reg, uint16_t data);
int16_t read_temp_raw(void);
float read_temp_celsius(void);
float read_temp_fahrenheit(void);
float calc_temp_fahrenheit(float temperature_celsius);
float get_high_limit(void);
void set_high_limit(float high_limit);
float get_low_limit(void);
void set_low_limit(float low_limit);
float get_temp_offset(void);
void set_temp_offset(float offset);
tmp117_flags_t get_status_flags(void); // get high, low, and data ready flags at once since these all clear when read
bool data_ready(void); // handy read data_ready flag, but also clears the high alert and low alert flags
uint8_t get_conversion_mode(void);
void set_continuous_conversion_mode(void);
void set_shutdown_mode(void);
void set_oneshot_mode(void);
uint8_t get_conversion_cycle_bit(void);
void set_conversion_cycle(tmp117_conversion_cycle_t cycle_time);
uint8_t get_conversion_averaging_mode(void);
void set_averaging_mode(tmp117_avg_mode_t mode);
bool get_thermalert_mode(void);
void set_thermalert_mode(tmp117_alert_mode_t mode);
bool get_pol_pin(void);
void set_pol_pin(uint8_t pol);
bool get_alert_pin(void);
void set_alert_pin(tmp117_alert_pin_select_t set_alert_pin);
void soft_reset(void);
int send_general_call_reset(void);
uint8_t send_smbus_alert(void);

// EEPROM function prototypes ("program" means write the setting to EEPROM)
bool eeprom_unlock(void);
bool eeprom_busy(void);
bool eeprom_write_register(uint16_t reg, uint16_t value);
bool eeprom_write_register_float(uint16_t reg, float value);
bool program_high_limit (float high_limit);
bool program_low_limit (float low_limit);
bool program_offset (float offset);
bool program_continuous_conversion_mode(void);
bool program_shutdown_mode(void);
bool program_conversion_cycle(tmp117_conversion_cycle_t cycle_time);
bool program_averaging_mode(tmp117_avg_mode_t mode);
bool program_thermalert_mode(tmp117_alert_mode_t set_alert_mode);
bool program_pol_pin(uint8_t pol);
bool program_alert_pin(tmp117_alert_pin_select_t set_alert_pin);
bool program_eeprom1 (uint16_t value);
bool program_eeprom2 (uint16_t value);
bool program_eeprom3 (uint16_t value);

#endif  // TMP117_H
