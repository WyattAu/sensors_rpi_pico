// Mock tmp117_registers.h for testing
#ifndef TMP117_REGISTERS_H
#define TMP117_REGISTERS_H

// Mock register definitions
#define TMP117_TEMP_RESULT 0x00
#define TMP117_CONFIGURATION 0x01
#define TMP117_T_HIGH_LIMIT 0x02
#define TMP117_T_LOW_LIMIT 0x03
#define TMP117_EEPROM_UL 0x04
#define TMP117_EEPROM1 0x05
#define TMP117_EEPROM2 0x06
#define TMP117_TEMP_OFFSET 0x07
#define TMP117_EEPROM3 0x08
#define TMP117_DEVICE_ID 0x0F

// Mock status codes
#define TMP117_OK 0
#define TMP117_ID_NOT_FOUND -2

// Mock timeout
#define SMBUS_TIMEOUT_US 100000

#endif
