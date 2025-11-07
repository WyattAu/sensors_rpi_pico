/**
 * Copyright (c) 2024 breaker
 * Based on code from:
 * - Raspberry Pi Pico SDK (c) 2020 Raspberry Pi (Trading) Ltd. (BSD-3-Clause License)
 * - SparkFun TMP117 Arduino Library (c) 2016 SparkFun Electronics (originally MIT, relicensed BSD-3-Clause)
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Original SparkFun TMP117 Arduino Library file: SparkFunTMP117.cpp

// This version is for use as an INTERFACE library to the Raspberry Pi Pico SDK

#include "tmp117.h"
#include "tmp117_registers.h"
#include "pico/stdlib.h"
#include "pico/printf.h"
#include "hardware/i2c.h"

// Static variable to hold the TMP117 address
static uint8_t tmp117_address = TMP117_DEFAULT_ADDRESS;
// variable to set the I2C instance to be used by tmp117_set_instance()
i2c_inst_t *i2c_instance = i2c_default;  // Default to board file I2C instance
// Track the last used pointer register, 0x09 as an invalid initial value
static uint8_t last_pointer = 0x09;

/**
 * @brief Set the I2C address for the TMP117 sensor.
 * 
 * This function assigns the I2C address to the TMP117 sensor. The address
 * is stored in the global variable `tmp117_address`.
 * 
 * @param address The I2C address of the TMP117 sensor.
 */
void tmp117_set_address(uint8_t address) {
    tmp117_address = address;
}

/**
 * @brief Get the I2C address of the TMP117 sensor.
 * 
 * This function returns the current I2C address stored in `tmp117_address`.
 * 
 * @return uint8_t The current I2C address of the TMP117 sensor.
 */
uint8_t tmp117_get_address(void) {
    return tmp117_address;
}

/**
 * @brief Set the I2C instance (i2c0 or i2c1) for the TMP117 communication.
 * 
 * This function assigns the specified I2C instance to the global `i2c_instance` 
 * variable for use in I2C communication with the TMP117 sensor.
 * 
 * @param instance Pointer to the I2C instance (i2c0 or i2c1).
 */
void tmp117_set_instance(i2c_inst_t *instance) {
    i2c_instance = instance;
}

/**
 * @brief Initialize the TMP117 sensor and check for correct device ID.
 * 
 * This function ensures that the TMP117 sensor acknowledges its I2C address 
 * and returns the correct device ID. It attempts to write to the device ID 
 * register and checks the device ID to confirm the correct sensor is connected.
 * 
 * @return int Returns TMP117_OK if successful, or an error code if failed.
 */
int begin(void) {
    uint8_t reg = TMP117_DEVICE_ID;

    int ret = i2c_write_timeout_us(i2c_instance, tmp117_address, &reg, 1, true, SMBUS_TIMEOUT_US);
    if (ret < 0) {
        return ret;
    }

    last_pointer = reg;

    if (read_register(TMP117_DEVICE_ID) != DEVICE_ID_VALUE) {
        return TMP117_ID_NOT_FOUND;
    }

    return TMP117_OK;
}

/**
 * @brief Read a 16-bit register value from the TMP117 using its device address.
 * 
 * This function reads a 16-bit value from the specified register of the TMP117.
 * It checks if the register address differs from the last accessed one, and 
 * if so, sets the register pointer before reading the data.
 * 
 * @param reg The register address to read from.
 * @return uint16_t The 16-bit unsigned integer value read from the register.
 */
uint16_t read_register(uint8_t reg) {
    uint8_t buffer[2] = {0};
    if (reg != last_pointer) {
        i2c_write_blocking(i2c_instance, tmp117_address, &reg, 1, true);
        last_pointer = reg;
    }
    i2c_read_blocking(i2c_instance, tmp117_address, buffer, 2, false);
    return (buffer[0] << 8) | buffer[1];
}

/**
 * @brief Write data to a register of the TMP117 using its device address.
 * 
 * This function writes a 16-bit data value to the specified register of the TMP117.
 * It splits the 16-bit value into two bytes and sends them to the TMP117 over I2C.
 * 
 * @param reg The register address to write to.
 * @param data The 16-bit data value to write to the register.
 */
void write_register(uint8_t reg, uint16_t data) {
    uint8_t buffer[3] = {reg, ((data >> 8) & 0xFF), (data & 0xFF)};
    i2c_write_blocking(i2c_instance, tmp117_address, buffer, 3, false);
    last_pointer = reg;
}

/**
 * @brief Get the raw temperature value from the TMP117 temperature result register.
 * 
 * This function reads the raw temperature value (16-bit) from the TMP117's 
 * temperature result register. The result is returned as an integer in Q7 
 * notation for further conversion.
 * 
 * @return int16_t The raw temperature result in Q7 notation.
 */
int16_t read_temp_raw(void) {
    return (int16_t)read_register(TMP117_TEMP_RESULT);
}

/**
 * @brief Get the temperature in Celsius from the TMP117 temperature result register.
 * 
 * This function reads the raw temperature value from the TMP117 temperature 
 * result register, converts it to Celsius, and returns the result as a float.
 * The conversion uses the TMP117 resolution of 7.8125 m°C.
 * 
 * @return float The temperature in degrees Celsius.
 */
float read_temp_celsius(void) {
    return (int16_t)read_register(TMP117_TEMP_RESULT) * TMP117_RESOLUTION;
}

/**
 * @brief Get the temperature in Fahrenheit from the TMP117 temperature result register.
 * 
 * This function reads the temperature in Celsius from the TMP117, converts it to 
 * Fahrenheit, and returns the result as a float.
 * 
 * @return float The temperature in degrees Fahrenheit.
 */
float read_temp_fahrenheit(void) {
    float temperature_celsius = read_temp_celsius();
    return 9.0 / 5.0 * temperature_celsius + 32;
}

/**
 * @brief Convert temperature from Celsius to Fahrenheit.
 * 
 * This function converts a given temperature in Celsius to Fahrenheit.
 * 
 * @param temperature_celsius The temperature in degrees Celsius.
 * @return float The converted temperature in degrees Fahrenheit.
 */
float calc_temp_fahrenheit(float temperature_celsius) {
    return 9.0 / 5.0 * temperature_celsius + 32;
}

/**
 * @brief Get the high limit temperature value from the TMP117 high limit register.
 * 
 * This function reads the high limit register of the TMP117, converts the value 
 * to degrees Celsius using the TMP117 resolution, and returns the result as a float.
 * 
 * @return float The high limit temperature in degrees Celsius.
 */
float get_high_limit(void) {
    return (int16_t)read_register(TMP117_T_HIGH_LIMIT) / 128.0;
}

/**
 * @brief Set the high limit temperature value in the TMP117 high limit register.
 * 
 * This function allows the user to set the high limit register to a specified 
 * temperature value in degrees Celsius. The value is first converted into the 
 * correct format for the TMP117 before being written to the register.
 * 
 * @param high_limit The high limit temperature in degrees Celsius.
 */
void set_high_limit(float high_limit) {
    float final_limit_float = high_limit / TMP117_RESOLUTION;
    uint16_t final_limit = (int16_t)final_limit_float;
    write_register(TMP117_T_HIGH_LIMIT, final_limit);
}

/**
 * @brief Get the low limit temperature value from the TMP117 low limit register.
 * 
 * This function reads the low limit register of the TMP117, converts the value 
 * to degrees Celsius using the TMP117 resolution, and returns the result as a float.
 * 
 * @return float The low limit temperature in degrees Celsius.
 */
float get_low_limit(void) {
    return (int16_t)read_register(TMP117_T_LOW_LIMIT) / 128.0;
}

/* SET LOW LIMIT
	This function allows the user to set the low limit register to whatever
	specified value, as long as in the range for the temperature  This
	function can be used as a threshold for Therm mode and or Alert mode.
	The values are signed integers since they can be negative.
*/
void set_low_limit(float low_limit) {
	// Divides by the TMP117 resolution to send the correct value to the register
	float final_limit_float = low_limit / TMP117_RESOLUTION;
	uint16_t final_limit = (int16_t)final_limit_float;
	// Writes to the low limit temperature register with the new limit value
	write_register(TMP117_T_LOW_LIMIT, final_limit);
}

// return Temperature Offset register value * TMP117_RESOLUTION
float get_temp_offset(void) {
    return (int16_t)read_register(TMP117_TEMP_OFFSET) * TMP117_RESOLUTION;
}

/**
 * @brief Set the temperature offset of the TMP117 sensor.
 *
 * This function allows the user to set an offset temperature for the TMP117 sensor.
 * The offset is calculated by dividing the desired temperature offset by the sensor's
 * resolution, then written to the TMP117_TEMP_OFFSET register (0x07).
 * 
 * @param offset The desired temperature offset in degrees Celsius.
 */
void set_temp_offset(float offset) {
    // Divides by the resolution to send the correct value to the sensor
    float final_offset_float = offset / TMP117_RESOLUTION;  // Calculate the offset in the correct unit
    uint16_t final_offset = (int16_t)final_offset_float;    // Typecast to int16_t, convert to two's complement
    // Write the new offset value to the temperature offset register
    write_register(TMP117_TEMP_OFFSET, final_offset);
}

/**
 * @brief Get High Alert, Low Alert, and Data Ready Status Flags
 * 
 * This function reads bits [15:13] of the configuration register to retrieve the 
 * high alert, low alert, and data ready status flags before they are cleared by other reads.
 * It returns a `TMP117_flags_t` struct with each status flag represented as a boolean.
 *
 * @return TMP117_flags_t struct with:
 * - `high_alert` set to true if the high alert flag is active, false otherwise.
 * - `low_alert` set to true if the low alert flag is active, false otherwise.
 * - `data_ready` set to true if the data ready flag is active, false otherwise.
 */
tmp117_flags_t get_status_flags(void) {
    uint16_t configuration_register = read_register(TMP117_CONFIGURATION); // Read the configuration register

    tmp117_flags_t flags;
    flags.high_alert = (configuration_register >> 15) & 0x01;  // Extract bit 15
    flags.low_alert = (configuration_register >> 14) & 0x01;   // Extract bit 14
    flags.data_ready = (configuration_register >> 13) & 0x01;  // Extract bit 13

    return flags;
}

/**
 * @brief Check if Data Ready Flag is Set
 * 
 * Reads the Data Ready flag (bit 13) of the configuration register.
 * 
 * @return 
 * - true  if the conversion is complete (Data Ready flag is set)
 * - false if the conversion is not complete
 *
 * @note Reading the temperature register or configuration register clears the Data Ready flag.
 */
bool data_ready(void) {
    // Get the latest status flags
    tmp117_flags_t flags = get_status_flags();
    
    // Return the Data Ready status
    return flags.data_ready;
}

/**
 * @brief Get the current conversion mode (MOD[1:0]) from the TMP117 configuration register.
 * 
 * This function reads the TMP117 configuration register and extracts the MOD[1:0] 
 * field (bits 11:10), which indicates the sensor's conversion mode:
 * - `00`: Continuous conversion (CC)
 * - `01`: Shutdown (SD)
 * - `10`: Continuous conversion (CC), same as `00`
 * - `11`: One-shot conversion (OS)
 * 
 * @return uint8_t The conversion mode value (0-3).
 */
uint8_t get_conversion_mode(void) {
    uint16_t configuration_register = read_register(TMP117_CONFIGURATION);
    return (configuration_register >> 10) & 0x03;
}

/**
 * @brief Set conversion mode to continuous conversion.
 * 
 * @note MOD[1:0] bits are set to 00 in the configuration register (refer to Section 7.4.1).
 * 
 * This function configures the TMP117 sensor to operate in continuous conversion mode. 
 * In this mode, the TMP117 continuously performs temperature conversions. Each conversion cycle consists of 
 * an active conversion period followed by a low-power standby period. The duration of these periods can be 
 * configured using the **CONV[2:0]** and **AVG[1:0]** bits in the configuration register.
 * 
 * The sensor updates the temperature result register at the end of each active conversion. The user can monitor 
 * the Data_Ready flag, which indicates when a conversion is complete. The Data_Ready flag can be cleared by reading 
 * either the configuration register or the temperature result register. The user can set the **DR/nAlert_EN** bit to 
 * monitor the Data_Ready flag on the ALERT pin.
 * 
 * The user can check the Data_Ready flag using `get_status_flags()` or `data_ready()`.
 *
 * @note Reading the configuration register clears the High Alert and Low Alert flags.
 *       Reading the configuration register or the temperature result register clears the Data Ready flag.
 *
 * @see Datasheet Section 7.4.1
 */
void set_continuous_conversion_mode(void) {
	uint16_t mode = read_register(TMP117_CONFIGURATION);
    mode &= ~(0b11 << 10); // clears bits 11 and 10
	write_register(TMP117_CONFIGURATION, mode);
}

/**
 * @brief Set conversion mode to shutdown.
 * 
 * @note MOD[1:0] bits are set to 01 in the configuration register.
 * 
 * This function configures the TMP117 sensor to enter shutdown mode. In this mode, 
 * the TMP117 ceases conversions and enters a low-power state.
 */
void set_shutdown_mode(void) {
	uint16_t mode = read_register(TMP117_CONFIGURATION);
	mode &= ~(1 << 11); // clears bit 11
	mode |= (1 << 10);  // sets bit 10 to 1
	write_register(TMP117_CONFIGURATION, mode);
}

/**
 * @brief Set conversion mode to one-shot.
 * 
 * @note MOD[1:0] bits are set to 11 in the configuration register.
 * 
 * This function configures the TMP117 sensor to operate in one-shot conversion mode.
 * In this mode, the TMP117 performs a single temperature conversion and then enters 
 * low-power shutdown mode. The conversion cycle time is determined by the AVG bits,
 * and the conversion duration is not affected by the CONV bits. The Data_Ready flag
 * is set at the end of the conversion, signaling the completion of the measurement.
 * 
 * The user can check the Data_Ready flag using `get_status_flags()` or `data_ready()`.
 * The DR/nAlert_EN bit can also be set to monitor the Data_Ready flag on the ALERT pin.
 *
 * @note Reading the configuration register clears the High Alert and Low Alert flags.
 *       Reading the configuration register or the temperature result register clears
 *       the Data Ready flag.
 */
void set_oneshot_mode(void) {
	uint16_t mode = read_register(TMP117_CONFIGURATION);
    mode |= (0b11 << 10); // sets bits 11 and 10 to 1
	write_register(TMP117_CONFIGURATION, mode);
}

/**
 * @brief Get the Therm/Alert mode (T/nA) value from the TMP117 configuration register.
 * 
 * This function reads the Therm/Alert mode select field (bit 4) from the TMP117 configuration register:
 * - `1`: Therm mode.
 * - `0`: Alert mode.
 * 
 * @return bool The Therm/Alert mode (`true` for Therm mode, `false` for Alert mode).
 */
bool get_thermalert_mode(void) {
    uint16_t configuration_register = read_register(TMP117_CONFIGURATION);
    return (configuration_register >> 4) & 0x01;
}

/**
 * @brief Sets Therm or Alert mode (T/nA bit in Configuration Register).
 * 
 * This function configures the TMP117 alert mode by setting or clearing bit 4:
 * - `THERM_MODE` (1): Therm mode
 *    - HIGH_Alert flag set if temperature > high limit; cleared if < low limit.
 *    - Creates hysteresis between high and low limits.
 * - `ALERT_MODE` (0): Alert mode
 *    - HIGH_Alert flag set if temperature > high limit.
 *    - LOW_Alert flag set if temperature < low limit.
 *    - Device is like a window limit detector in Alert mode.
 *
 * In both modes:
 * - The ALERT pin behavior and status flags are affected.
 * - I2C reads do not affect flags in Therm mode but clear them in Alert mode.
 * 
 * @param mode Can be set to `THERM_MODE` (1) for Therm mode or `ALERT_MODE` (0) for Alert mode.
 */
void set_thermalert_mode(tmp117_alert_mode_t set_alert_mode) {
    uint16_t configuration_register = read_register(TMP117_CONFIGURATION);

    if (set_alert_mode == THERM_MODE) { // 1: Therm mode
        configuration_register |= (0x01 << 4); // Set bit 4 to 1
    } else { // 0: Alert mode
        configuration_register &= ~(0x01 << 4); // Clear bit 4
    }
    // Write updated configuration register
    write_register(TMP117_CONFIGURATION, configuration_register);
}

/**
 * @brief Get the ALERT pin polarity bit (POL) value from the TMP117 configuration register.
 * 
 * This function reads the ALERT pin polarity (bit 3, POL) from the TMP117 configuration register:
 * - `1`: ALERT pin is active high.
 * - `0`: ALERT pin is active low.
 * 
 * @return bool The ALERT pin polarity (`true` for active high, `false` for active low).
 */
bool get_pol_pin(void) {
    uint16_t configuration_register = read_register(TMP117_CONFIGURATION);
    return (configuration_register >> 3) & 0x01;
}

/**
 * @brief Set the ALERT pin polarity (POL) value in the TMP117 configuration register.
 * 
 * This function sets the ALERT pin polarity (bit 3, POL) in the TMP117 configuration register:
 * - `1`: Set ALERT pin to active high.
 * - `0`: Set ALERT pin to active low.
 * 
 * @param pol The desired polarity (`1` for active high, `0` for active low).
 */
void set_pol_pin(uint8_t pol) {
    uint16_t configuration_register = read_register(TMP117_CONFIGURATION);

    if (pol == 1) {
        configuration_register |= (0x01 << 3); // Set bit 3 of the configuration register to 1
    } else {
        configuration_register &= ~(0x01 << 3); // Clear bit 3 of the configuration register
    }

    // Write updated configuration register
    write_register(TMP117_CONFIGURATION, configuration_register);
}

/**
 * @brief Get the ALERT pin select bit value from the TMP117 configuration register.
 * 
 * This function reads the ALERT pin select bit (bit 2) to determine its function:
 * - `1`: ALERT pin reflects the status of the data ready flag.
 * - `0`: ALERT pin reflects the status of the alert flags.
 * 
 * @return bool The ALERT pin select value (`true` for data ready, `false` for alert flags).
 */
bool get_alert_pin(void) {
    uint16_t configuration_register = read_register(TMP117_CONFIGURATION);
    return (configuration_register >> 2) & 0x01;
}

/**
 * @brief Set the ALERT pin function in the TMP117 configuration register.
 * 
 * This function configures the ALERT pin (bit 2) function:
 * - `ALERT_PIN_DATA_READY` (`1`): ALERT pin reflects the status of the data ready flag.
 * - `ALERT_PIN_ALERT_FLAGS` (`0`): ALERT pin reflects the status of the alert flags.
 * 
 * @param set_alert_pin The desired ALERT pin function.
 */
void set_alert_pin(tmp117_alert_pin_select_t set_alert_pin) {
    uint16_t configuration_register = read_register(TMP117_CONFIGURATION);

    if (set_alert_pin == ALERT_PIN_DATA_READY) {
        configuration_register |= (0x01 << 2); // Set bit 2 of the configuration register to 1
    } else {
        configuration_register &= ~(0x01 << 2); // Clear bit 2 of the configuration register
    }

    write_register(TMP117_CONFIGURATION, configuration_register);
}

/**
 * @brief Get the Conversion Averaging Mode (AVG[1:0]) from the Configuration Register
 * 
 * This function retrieves the Conversion Averaging mode bits (AVG[1:0]) from the TMP117 configuration register. 
 * These bits determine how many temperature conversions are averaged before updating the temperature register.
 * 
 * @return uint8_t The 2-bit averaging mode value (bits 6 and 5) from the configuration register.
 */
uint8_t get_conversion_averaging_mode(void) {
    uint16_t configuration_register = read_register(TMP117_CONFIGURATION);
    return (configuration_register >> 5) & 0x03; // Extract bits 6 and 5 
}

/**
 * @brief Set Conversion Averaging Mode (AVG[1:0] bits 6:5 in the Configuration Register)
 * 
 * This function configures the TMP117 to report the average of multiple temperature conversions to reduce noise 
 * in the conversion results. The device supports the following conversion averaging modes, determined by setting the 
 * AVG[1:0] bits in the configuration register:
 * - 00: No averaging (single conversion result)
 * - 01: Average of 8 conversions
 * - 10: Average of 32 conversions
 * - 11: Average of 64 conversions
 * 
 * In these modes, the device collects the specified number of conversion results and calculates an accumulated 
 * average (not a running average) to update the temperature register. Averaging improves noise performance, with a 
 * repeatability of approximately ±1 LSB when averaging 8 conversions, compared to ±3 LSB without averaging.
 * 
 * Using averaging increases the conversion cycle time, as the active conversion time is multiplied by the number 
 * of conversions in the average. For example, averaging 8 conversions results in an active conversion time of 
 * 124 ms (15.5 ms × 8). Averaging can be used in both continuous conversion mode and one-shot mode.
 * 
 * @param mode The averaging mode to set, corresponding to the number of conversions to average:
 *        AVG_NONE      : No averaging
 *        AVG_8         : Average of 8 conversions
 *        AVG_32        : Average of 32 conversions
 *        AVG_64        : Average of 64 conversions
 * 
 * @note Averaging increases the conversion cycle time and thus impacts the active current consumption.
 */
void set_averaging_mode(tmp117_avg_mode_t mode) {
    uint16_t configuration_register = read_register(TMP117_CONFIGURATION);

    // Clear bits 5 and 6, then set them based on the mode
    configuration_register &= ~(0b11 << 5);
    configuration_register |= (mode << 5);

    write_register(TMP117_CONFIGURATION, configuration_register);
}

/**
 * @brief Get the Conversion Cycle Bits (CONV[2:0]) from the Configuration Register
 * 
 * This function retrieves the Conversion Cycle bits (CONV[2:0]) from the TMP117 configuration register. 
 * These bits determine the conversion cycle time in continuous conversion mode.
 * 
 * @return uint8_t The 3-bit conversion cycle value (bits 9, 8, and 7) from the configuration register.
 *
 * @see Table 7-7 of the datasheet
 */
uint8_t get_conversion_cycle_bit(void) {
	uint16_t configuration_register = read_register(TMP117_CONFIGURATION);
    // Extract bits 9, 8, and 7 using bitwise operations and masking
    return (configuration_register >> 7) & 0x07;
}

/**
 * @brief Conversion Cycle Time in Continuous Conversion (CC) Mode
 * 
 * Table 7-7 shows the conversion cycle times in continuous conversion (CC) mode, depending on the settings of 
 * the CONV[2:0] and AVG[1:0] bits in the configuration register.
 *
 * - The **CONV[2:0] bits** determine the base duration of the conversion cycle.
 * - The **AVG[1:0] bits** control the number of temperature readings averaged before the temperature register is updated.
 * 
 * The table shows the resulting conversion cycle times for various combinations of these bits:
 * 
 * | CONV[2:0] | AVG[1:0] = 00 | AVG[1:0] = 01 | AVG[1:0] = 10 | AVG[1:0] = 11 |
 * |-----------|----------------|----------------|----------------|----------------|
 * | 000       | 15.5 ms        | 125 ms         | 500 ms         | 1 s            |
 * | 001       | 125 ms         | 125 ms         | 500 ms         | 1 s            |
 * | 010       | 250 ms         | 250 ms         | 500 ms         | 1 s            |
 * | 011       | 500 ms         | 500 ms         | 500 ms         | 1 s            |
 * | 100       | 1 s            | 1 s            | 1 s            | 1 s            |
 * | 101       | 4 s            | 4 s            | 4 s            | 4 s            |
 * | 110       | 8 s            | 8 s            | 8 s            | 8 s            |
 * | 111       | 16 s           | 16 s           | 16 s           | 16 s           |
 * 
 * Depending on the specific application needs, users can select the appropriate **CONV** and **AVG** settings to balance 
 * conversion time, noise reduction (via averaging), and power consumption.
 * 
 * @note Longer conversion cycle times with higher averaging modes result in better noise performance but increase the 
 *       overall cycle time and the active conversion duration, which affects current consumption.
 */
void set_conversion_cycle(tmp117_conversion_cycle_t cycle_time) {
    // Read the current configuration register value
    uint16_t configuration_register = read_register(TMP117_CONFIGURATION);
    
    configuration_register &= ~(0x07 << 7); // Clear conversion cycle bits
    configuration_register |= (cycle_time << 7); // Set new conversion cycle bits
    
    // Write the updated configuration back to the register
    write_register(TMP117_CONFIGURATION, configuration_register);
}

/**
 * @brief Triggers a 2 ms software reset by setting bit 1 of the configuration register.
 * 
 * Sets bit 1 of the configuration register (0x01) to 1 to initiate a reset, 
 * which lasts 2 ms. After the reset, bit 1 will read back as 0. The `last_pointer` 
 * is reset to 0x00 after completion to reflect pointer state after a reset.
 * 
 * @note TMP117_RESET_DELAY_MS is 2ms, adjust as needed in tmp117.h
 *
 * @see TMP117 datasheet for more details.
 */
void soft_reset(void) {
    // read into a variable value of configuration register (01h)
    uint16_t configuration_register = read_register(TMP117_CONFIGURATION);
    // change bit 1 to 1 for soft reset
    configuration_register |= (1 << 1);
    // write new configuration register value
    write_register(TMP117_CONFIGURATION, configuration_register);
    // update last pointer to reset value (00)
    last_pointer = 0x00;
    // wait for TMP117 to reset before exiting function
    sleep_ms(TMP117_RESET_DELAY_MS);
}

/**
 * @brief Sends a general call reset command via I2C.
 *
 * This function sends a general call reset command (0x06) to all devices on 
 * the I2C bus using the general call address (0x00). The command resets all 
 * I2C devices that respond to the general call. It waits for a short period 
 * of time after sending the reset command to allow the TMP117 sensor to reset.
 *
 * @return int Returns the number of bytes written, or PICO_ERROR_GENERIC 
 * if the reset command was not acknowledged or no device is present.
 */
int send_general_call_reset(void) {
    const uint8_t general_call_addr = 0x00;
    const uint8_t reset_command = 0x06;
    
    int result = i2c_write_blocking(i2c_instance, general_call_addr, &reset_command, 1, false);

    if (result == 1) {
        sleep_ms(TMP117_RESET_DELAY_MS);
        last_pointer = 0x00;
    }
    
    return result; // should be 1 if successful
}

/**
 * @brief Send an SMBus Alert Response and return the address of the responding device.
 * 
 * This function sends an SMBus Alert Response command by performing an I2C read operation 
 * to the SMBus Alert Response Address (`0x0C`). The responding device returns its 7-bit 
 * address along with the least significant bit (LSB) indicating its alert type:
 * - `1`: High alert.
 * - `0`: Low alert.
 * 
 * @note The function uses a timeout to handle potential communication delays.
 * 
 * @return uint8_t The 8-bit response containing the 7-bit device address and the alert type (LSB).
 */
uint8_t send_smbus_alert(void) {
    const uint8_t alert_response_address = 0x0C;
    uint8_t buffer = 0;

    i2c_read_timeout_us(i2c_instance, alert_response_address, &buffer, 1, false, SMBUS_TIMEOUT_US);

    return buffer;
}

// All of the EEPROM functions are below this comment, except send_general_call_reset()

// Check if the EUN (EEPROM unlock) flag is set
bool eeprom_unlock(void) {
    uint16_t eeprom_unlock_reg = read_register(TMP117_EEPROM_UL);
    return (eeprom_unlock_reg >> 15) & 0x01;
}

// Check if the EEPROM_Busy flag is set
bool eeprom_busy(void) {
    // read the value of the EEPROM unlock register
    uint16_t unlock_register = read_register(TMP117_EEPROM_UL);
    // return the value of bit 14 (mirror of configuration register EEPROM busy flag)
    return (unlock_register >> 14) & 0x01;
}

/**
 * @brief Writes a floating-point value to a TMP117 EEPROM register.
 *
 * @note Used to program EEPROM registers such temperature offset, high limit, and low limit.
 *
 * @param reg The address of the EEPROM register to write to.
 * @param value The floating-point value to write, converted based on the TMP117 resolution.
 * 
 * @return bool Returns true if the value written matches the value read back 
 * from the EEPROM, indicating a successful write, otherwise returns false.
 */
bool eeprom_write_register_float(uint16_t reg, float value) {
    // Convert the float value to an appropriate unsigned 16-bit value for the sensor
    float float_value = value / TMP117_RESOLUTION;
    uint16_t final_value = (int16_t)float_value;
    set_shutdown_mode();                                    // Ensure no conversions are taking place
    write_register(TMP117_EEPROM_UL, EEPROM_UNLOCK_VALUE);  // set bit 15 of EEPROM_UL to 1 to unlock EEPROM
    write_register(reg, final_value);                       // Program the EEPROM with the final value

    // Wait until EEPROM is no longer busy
    do {
        sleep_ms(7);
    } while (eeprom_busy() == true);

    // Issue an I2C general call reset
    send_general_call_reset();

    // Verify if the write was successful by reading back the value
    uint16_t current_value = read_register(reg);
    return (current_value == final_value);
}

// program an eeprom with an unsigned 16-bit value
bool eeprom_write_register(uint16_t reg, uint16_t value) {
    set_shutdown_mode();                                    // Ensure no conversions are taking place
    write_register(TMP117_EEPROM_UL, EEPROM_UNLOCK_VALUE);  // set bit 15 of EEPROM_UL to 1 to unlock EEPROM
    write_register(reg, value);                             // Program the EEPROM with the value

    // Wait until EEPROM is no longer busy
    do {
        sleep_ms(7);
    } while (eeprom_busy() == true);

    // Issue an I2C general call reset
    send_general_call_reset();

    // Verify if the write was successful by reading back the value
    uint16_t current_value = read_register(reg);
    return (current_value == value);
}

// program temperature high limit register
bool program_high_limit (float high_limit) {
    return eeprom_write_register_float(TMP117_T_HIGH_LIMIT, high_limit);
}

// program temperature low limit register
bool program_low_limit (float low_limit) {
    return eeprom_write_register_float(TMP117_T_LOW_LIMIT, low_limit);
}

// program offset register
bool program_offset (float offset) {
    return eeprom_write_register_float(TMP117_TEMP_OFFSET, offset);
}

bool program_continuous_conversion_mode(void) {
	uint16_t mode = read_register(TMP117_CONFIGURATION);
    mode &= ~(0b11 << 10); // clears bits 11 and 10
	return eeprom_write_register(TMP117_CONFIGURATION, mode);
}

// program shutdown mode (you cannot program one-shot mode as a default startup mode)
bool program_shutdown_mode(void) {
	uint16_t mode = read_register(TMP117_CONFIGURATION);
	mode &= ~(1 << 11); // sets bit 11 to 0
	mode |= (1 << 10);  // sets bit 10 to 1
	return eeprom_write_register(TMP117_CONFIGURATION, mode);
}

bool program_conversion_cycle(tmp117_conversion_cycle_t cycle_time) {
    // Read the current configuration register value
    uint16_t configuration_register = read_register(TMP117_CONFIGURATION);
    
    configuration_register &= ~(0x07 << 7); // Clear conversion cycle bits
    configuration_register |= (cycle_time << 7); // Set new conversion cycle bits
    
    // Write the updated configuration back to the register
    return eeprom_write_register(TMP117_CONFIGURATION, configuration_register);
}

// program averaging mode to EEPROM
bool program_averaging_mode(tmp117_avg_mode_t mode) {
    uint16_t configuration_register = read_register(TMP117_CONFIGURATION);

    // Clear bits 5 and 6, then set them based on the mode
    configuration_register &= ~(0b11 << 5);
    configuration_register |= (mode << 5);

    return eeprom_write_register(TMP117_CONFIGURATION, configuration_register);
}

// program `THERM_MODE` (1) for Therm mode or `ALERT_MODE` (0) for Alert mode.
bool program_thermalert_mode(tmp117_alert_mode_t set_alert_mode) {
    uint16_t configuration_register = read_register(TMP117_CONFIGURATION);

    if (set_alert_mode == THERM_MODE) { // 1: Therm mode
        configuration_register |= (0x01 << 4); // Set bit 4 to 1
    } else { // 0: Alert mode
        configuration_register &= ~(0x01 << 4); // Clear bit 4
    }

    return eeprom_write_register(TMP117_CONFIGURATION, configuration_register);
}

// Program ALERT pin polarity bit value (configuration register bit 2 POL)
/*  1: Active high
    0: Active low */
bool program_pol_pin(uint8_t pol) {
    uint16_t configuration_register = read_register(TMP117_CONFIGURATION);

    if (pol == 1) {
        configuration_register |= (0x01 << 3); // Set bit 3 of the configuration register to 1
    } else {
        configuration_register &= ~(0x01 << 3); // Clear bit 3 of the configuration register
    }

    // Write updated configuration register
    return eeprom_write_register(TMP117_CONFIGURATION, configuration_register);
}

bool program_alert_pin(tmp117_alert_pin_select_t set_alert_pin) {
    uint16_t configuration_register = read_register(TMP117_CONFIGURATION);

	if (set_alert_pin == ALERT_PIN_DATA_READY) {
        configuration_register |= (0x01 << 2); // set bit 2 of the configuration register to 1
    }
	else {
        configuration_register &= ~(0x01 << 2); // clear bit 2 of the configuration register
	}
    
    // Write updated configuration register
    return eeprom_write_register(TMP117_CONFIGURATION, configuration_register);
}

// program EEPROM1
bool program_eeprom1 (uint16_t value) {
    return eeprom_write_register(TMP117_EEPROM1, value);
}

// program EEPROM2
bool program_eeprom2 (uint16_t value) {
    return eeprom_write_register(TMP117_EEPROM2, value);
}

// program EEPROM3
bool program_eeprom3 (uint16_t value) {
    return eeprom_write_register(TMP117_EEPROM3, value);
}