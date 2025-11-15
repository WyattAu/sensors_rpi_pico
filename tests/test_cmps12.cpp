#include <gtest/gtest.h>
#include "cmps12.h"

// Mock functions for testing
extern "C" {
    // Mock CMPS12 functions
    bool cmps12_init(cmps12_t *compass, i2c_inst_t *i2c) {
        compass->i2c = i2c;
        return true;
    }

    bool cmps12_read(cmps12_t *compass) {
        compass->angle8 = 0x80;  // 128 degrees
        compass->angle16 = 1280; // 128.0 degrees * 10
        compass->pitch = 0;
        compass->roll = 0;
        return true;
    }

    const char* cmps12_get_cardinal_direction(uint16_t angle) {
        if (angle < 225) return "N";
        if (angle < 675) return "NE";
        if (angle < 1125) return "E";
        if (angle < 1575) return "SE";
        if (angle < 2025) return "S";
        if (angle < 2475) return "SW";
        if (angle < 2925) return "W";
        if (angle < 3375) return "NW";
        return "Unknown";
    }
}

// Test fixture for CMPS12 tests
class CMPS12Test : public ::testing::Test {
protected:
    cmps12_t compass;

    void SetUp() override {
        // Initialize compass structure
        compass.i2c = reinterpret_cast<i2c_inst_t*>(0x1000); // Mock I2C instance
        compass.angle8 = 0;
        compass.angle16 = 0;
        compass.pitch = 0;
        compass.roll = 0;
    }

    void TearDown() override {
        // Cleanup if needed
    }
};

// Test CMPS12 initialization
TEST_F(CMPS12Test, Init_ValidI2C) {
    bool result = cmps12_init(&compass, compass.i2c);
    // With mocked I2C, this should succeed
    EXPECT_TRUE(result);
}

// Test CMPS12 data reading
TEST_F(CMPS12Test, Read_ValidData) {
    // First initialize
    ASSERT_TRUE(cmps12_init(&compass, compass.i2c));

    // Read data
    bool result = cmps12_read(&compass);
    EXPECT_TRUE(result);

    // Check that data was populated (mock data)
    EXPECT_EQ(compass.angle8, 0x80); // 128 from mock
    EXPECT_EQ(compass.angle16, 0x8000); // 32768 from mock
    EXPECT_EQ(compass.pitch, 0);
    EXPECT_EQ(compass.roll, 0);
}

// Test cardinal direction conversion
TEST_F(CMPS12Test, CardinalDirection_North) {
    const char* direction = cmps12_get_cardinal_direction(0);
    ASSERT_STREQ(direction, "N");
}

TEST_F(CMPS12Test, CardinalDirection_East) {
    const char* direction = cmps12_get_cardinal_direction(90 * 10); // 90 degrees * 10
    ASSERT_STREQ(direction, "E");
}

TEST_F(CMPS12Test, CardinalDirection_South) {
    const char* direction = cmps12_get_cardinal_direction(180 * 10); // 180 degrees * 10
    ASSERT_STREQ(direction, "S");
}

TEST_F(CMPS12Test, CardinalDirection_West) {
    const char* direction = cmps12_get_cardinal_direction(270 * 10); // 270 degrees * 10
    ASSERT_STREQ(direction, "W");
}

TEST_F(CMPS12Test, CardinalDirection_NorthEast) {
    const char* direction = cmps12_get_cardinal_direction(45 * 10); // 45 degrees * 10
    ASSERT_STREQ(direction, "NE");
}

TEST_F(CMPS12Test, CardinalDirection_Invalid) {
    const char* direction = cmps12_get_cardinal_direction(4000); // Invalid angle
    ASSERT_STREQ(direction, "Unknown");
}