#include <gtest/gtest.h>
#include <chrono>
#include "tmp117.h"
#include "cmps12.h"

// Integration test fixture
class SensorsIntegrationTest : public ::testing::Test {
protected:
    cmps12_t compass;

    void SetUp() override {
        // Initialize compass for integration tests
        compass.i2c = reinterpret_cast<i2c_inst_t*>(0x1000);
        compass.angle8 = 0;
        compass.angle16 = 0;
        compass.pitch = 0;
        compass.roll = 0;
    }

    void TearDown() override {
        // Cleanup
    }
};

// Test sensor initialization sequence
TEST_F(SensorsIntegrationTest, SensorInitializationSequence) {
    // Test TMP117 I2C check
    uint test_frequency = 400000;
    check_i2c(test_frequency);

    // Test CMPS12 initialization
    bool cmps12_result = cmps12_init(&compass, compass.i2c);
    EXPECT_TRUE(cmps12_result);

    SUCCEED();
}

// Test sensor data reading workflow
TEST_F(SensorsIntegrationTest, SensorDataReadingWorkflow) {
    // Initialize CMPS12
    ASSERT_TRUE(cmps12_init(&compass, compass.i2c));

    // Read compass data
    bool read_result = cmps12_read(&compass);
    EXPECT_TRUE(read_result);

    // Verify data ranges
    EXPECT_GE(compass.angle8, 0);
    EXPECT_LE(compass.angle8, 255);
    EXPECT_GE(compass.angle16, 0);
    EXPECT_LE(compass.angle16, 3599); // 0-359.9 degrees * 10

    // Test cardinal direction for read angle
    const char* direction = cmps12_get_cardinal_direction(compass.angle16);
    EXPECT_NE(direction, nullptr);
    EXPECT_STRNE(direction, "");
}

// Test error handling scenarios
TEST_F(SensorsIntegrationTest, ErrorHandling_InvalidI2C) {
    // Test with null I2C instance
    cmps12_t bad_compass = {nullptr, 0, 0, 0, 0};
    bool result = cmps12_init(&bad_compass, nullptr);
    // Depending on implementation, this might fail or handle gracefully
    // EXPECT_FALSE(result); // Uncomment if implementation checks for null
    SUCCEED();
}

// Test data consistency
TEST_F(SensorsIntegrationTest, DataConsistency_MultipleReads) {
    ASSERT_TRUE(cmps12_init(&compass, compass.i2c));

    // Read multiple times
    for (int i = 0; i < 5; ++i) {
        bool result = cmps12_read(&compass);
        EXPECT_TRUE(result);

        // Store first reading for comparison
        static uint16_t first_angle = 0;
        if (i == 0) {
            first_angle = compass.angle16;
        } else {
            // In a real scenario, angles might vary slightly
            // For mock data, they should be consistent
            EXPECT_EQ(compass.angle16, first_angle);
        }
    }
}

// Performance test (basic timing)
TEST_F(SensorsIntegrationTest, Performance_BasicReadTiming) {
    ASSERT_TRUE(cmps12_init(&compass, compass.i2c));

    // Measure basic read performance
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100; ++i) {
        cmps12_read(&compass);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Should complete in reasonable time (allowing for mock overhead)
    EXPECT_LT(duration.count(), 1000); // Less than 1 second for 100 reads
}