#include "tmp117.h"
#include "tmp117_registers.h"
#include "pico/stdlib.h"
#include "pico/printf.h"
#include "hardware/i2c.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "cmps12.h"
#include "lib/pimoroni-pico/common/pimoroni_common.hpp"
#include "icp10125.hpp"

// I2C Configuration
#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5
#define I2C_FREQ 100000  // 100 kHz

// Optional: Calibration offset (uncomment and adjust as needed)
// #define CALIBRATION_OFFSET 335

#define SERIAL_INIT_DELAY_MS 2000      // Adjust as needed to mitigate garbage characters
#define I2C_SDA PICO_DEFAULT_I2C_SDA_PIN  // Set to a different SDA pin as needed
#define I2C_SCL PICO_DEFAULT_I2C_SCL_PIN  // Set to a different SCL pin as needed
#define TMP117_OFFSET_VALUE -25.0f     // Temperature offset in degrees C (for testing)
#define TMP117_CONVERSION_DELAY_MS 1000  // Adjust based on conversion cycle time

using namespace pimoroni;

I2C i2c(BOARD::BREAKOUT_GARDEN);
ICP10125 icp10125(&i2c);

int main(void) {
    // Initialize chosen interface
    stdio_init_all();
    // A little delay to ensure serial line stability
    sleep_ms(SERIAL_INIT_DELAY_MS);

    // Uncomment below to set I2C address other than 0x48 (e.g., 0x49)
    //tmp117_set_address(0x49);

    // Selects I2C instance (i2c_default is set as default in the tmp117.c)
    //tmp117_set_instance(i2c1); // change to i2c1 as needed

    // Initialize I2C (default i2c0) and get I2C frequency
    uint frequency = i2c_init(I2C_PORT, I2C_FREQ);

    // Configure the GPIO pins for I2C
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Call a function to check and report if I2C is running
    check_i2c(frequency);
 
    // Check if TMP117 is on the I2C bus at the address specified
    check_status(frequency);

    // TMP117 software reset; loads EEPROM Power On Reset values
    soft_reset();

    // Initialize compass
    cmps12_t compass;
    if (!cmps12_init(&compass, I2C_PORT)) {
        printf("Failed to initialize CMPS12!\n");
        while (1) {
            tight_loop_contents();
        }

    printf("CMPS12 initialized successfully!\n\n");

      icp10125.init();
      printf("init()\n");

    while (1) {
        // Read compass data
        if (cmps12_read(&compass)) {
            // Calculate angle in degrees with decimal
            int angle_whole = compass.angle16 / 10;
            int angle_decimal = compass.angle16 % 10;
            
            // Print all data
            printf("roll: %d    ", compass.roll);
            printf("pitch: %d    ", compass.pitch);
            printf("angle 8: %d    ", compass.angle8);
            printf("angle 16: %d.%d    ", angle_whole, angle_decimal);
            
            // Optional: Apply calibration offset
            #ifdef CALIBRATION_OFFSET
            int calibrated = (compass.angle16 - (CALIBRATION_OFFSET * 10) + 3600) % 3600;
            int cal_whole = calibrated / 10;
            int cal_decimal = calibrated % 10;
            printf("calibrated: %d.%d    ", cal_whole, cal_decimal);
            printf("direction: %s\n", cmps12_get_cardinal_direction(cal_whole));
            #else
            printf("direction: %s\n", cmps12_get_cardinal_direction(angle_whole));
            #endif
        } else {
            printf("Failed to read from CMPS12\n");
        }
        

        do {
            sleep_ms(TMP117_CONVERSION_DELAY_MS);
        } while (!data_ready()); // Check if the data ready flag is high

        /* 1) Typecast temp_result register to integer, converting from two's complement
           2) Multiply by 100 to scale the temperature (i.e. 2 decimal places)
           3) Shift right by 7 to account for the TMP117's 1/128 resolution (Q7 format) */
        int temp = read_temp_raw() * 100 >> 7;
        
        // Display the temperature in degrees Celsius, formatted to show two decimal places
        printf("Temperature: %d.%02d °C\n", temp / 100, (temp < 0 ? -temp : temp) % 100);

        // Floating point functions are also available for converting to Celsius or Fahrenheit
        //printf("\nTemperature: %.2f °C\t%.2f °F", read_temp_celsius(), read_temp_fahrenheit());
    
        auto result = icp10125.measure(ICP10125::NORMAL);
        printf("%fc %fPa %d\n", result.temperature, result.pressure, result.status);
        sleep_ms(500);
    }

    return 0;
}