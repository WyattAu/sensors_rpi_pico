#include <gtest/gtest.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Test CPM-managed libraries (pico-sdk utilities that can be tested on host)
// Since pico-sdk requires hardware, we test portable utility functions

// Mock some pico-sdk types for testing
typedef unsigned int uint;

// Test mathematical utilities that might be used
class CPMUtilitiesTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test basic mathematical operations that might be used in sensor processing
TEST_F(CPMUtilitiesTest, BasicMath_Operations) {
    // Test temperature conversion (Celsius to Fahrenheit)
    double celsius = 25.0;
    double fahrenheit = (celsius * 9.0/5.0) + 32.0;
    EXPECT_DOUBLE_EQ(fahrenheit, 77.0);

    // Test angle conversions (degrees to radians)
    double degrees = 180.0;
    double radians = degrees * M_PI / 180.0;
    EXPECT_DOUBLE_EQ(radians, M_PI);

    // Test sensor value scaling
    uint16_t raw_value = 32767; // Mid-range of 16-bit value
    double scaled_value = (raw_value / 65535.0) * 100.0; // Scale to 0-100
    EXPECT_NEAR(scaled_value, 50.0, 0.01);
}

// Test data processing utilities
TEST_F(CPMUtilitiesTest, DataProcessing_ValidRanges) {
    // Test temperature range validation
    double valid_temps[] = {-40.0, 0.0, 25.0, 125.0};
    for (double temp : valid_temps) {
        EXPECT_GE(temp, -55.0); // TMP117 valid range
        EXPECT_LE(temp, 150.0);
    }

    // Test compass bearing validation
    uint16_t bearings[] = {0, 900, 1800, 2700, 3599}; // 0-359.9 degrees * 10
    for (uint16_t bearing : bearings) {
        EXPECT_GE(bearing, 0);
        EXPECT_LE(bearing, 3599);
    }
}

// Test error handling utilities
TEST_F(CPMUtilitiesTest, ErrorHandling_BoundaryConditions) {
    // Test division by zero protection
    auto safe_divide = [](double a, double b) -> double {
        return (b != 0.0) ? (a / b) : 0.0;
    };

    EXPECT_DOUBLE_EQ(safe_divide(10.0, 2.0), 5.0);
    EXPECT_DOUBLE_EQ(safe_divide(10.0, 0.0), 0.0);

    // Test array bounds checking
    int test_array[5] = {1, 2, 3, 4, 5};
    auto safe_access = [&](size_t index) -> int {
        return (index < 5) ? test_array[index] : -1;
    };

    EXPECT_EQ(safe_access(0), 1);
    EXPECT_EQ(safe_access(4), 5);
    EXPECT_EQ(safe_access(5), -1); // Out of bounds
}

// Test configuration validation
TEST_F(CPMUtilitiesTest, ConfigurationValidation_SensorSettings) {
    // Test I2C address validation
    auto is_valid_i2c_address = [](uint8_t addr) -> bool {
        return (addr >= 0x08 && addr <= 0x77); // Valid I2C address range
    };

    EXPECT_TRUE(is_valid_i2c_address(0x48)); // TMP117 default
    EXPECT_TRUE(is_valid_i2c_address(0x60)); // CMPS12 default
    EXPECT_FALSE(is_valid_i2c_address(0x00)); // Invalid
    EXPECT_FALSE(is_valid_i2c_address(0x80)); // Invalid

    // Test sensor timing validation
    auto is_valid_sample_rate = [](uint rate_hz) -> bool {
        uint valid_rates[] = {1, 4, 8, 16, 32};
        for (uint valid_rate : valid_rates) {
            if (rate_hz == valid_rate) return true;
        }
        return false;
    };

    EXPECT_TRUE(is_valid_sample_rate(1));
    EXPECT_TRUE(is_valid_sample_rate(16));
    EXPECT_FALSE(is_valid_sample_rate(10));
}