
#include "lcd.h"
#include "pico/stdlib.h"
#include <stdint.h>

// Static variable to store the current configuration
static LCD_Config lcd_config;

// Initializes the LCD with the specified pin configuration
void lcd_pin_init(LCD_Config *config) {
    // Copy the provided configuration
    lcd_config = *config;

    // Initialize GPIO pins as outputs
    gpio_init(lcd_config.rs_pin);
    gpio_set_dir(lcd_config.rs_pin, GPIO_OUT);

    gpio_init(lcd_config.enable_pin);
    gpio_set_dir(lcd_config.enable_pin, GPIO_OUT);

    for (int i = 0; i < 4; i++) {
        gpio_init(lcd_config.data_pins[i]);
        gpio_set_dir(lcd_config.data_pins[i], GPIO_OUT);
    }
}

void lcd_pulse_enable(void) {
    gpio_put(lcd_config.enable_pin, 1);
    sleep_us(1);  // Enable pulse width > 450ns
    gpio_put(lcd_config.enable_pin, 0);
    sleep_us(50); // Commands need > 37µs to settle
}

void lcd_send_nibble(uint8_t nibble) {
    gpio_put(lcd_config.data_pins[0], (nibble >> 0) & 1);
    gpio_put(lcd_config.data_pins[1], (nibble >> 1) & 1);
    gpio_put(lcd_config.data_pins[2], (nibble >> 2) & 1);
    gpio_put(lcd_config.data_pins[3], (nibble >> 3) & 1);
    lcd_pulse_enable();
}

void lcd_send_byte(uint8_t rs, uint8_t data) {
    gpio_put(lcd_config.rs_pin, rs);  // RS=0 for command, 1 for data
    lcd_send_nibble(data >> 4); // Send higher nibble
    lcd_send_nibble(data & 0xF); // Send lower nibble
    sleep_us(50); // Wait for the command to execute
}

// Call lcd_pin_init() before calling lcd_init()
void lcd_init(void) {
    sleep_ms(15); // Wait for LCD to power up

    // Initialization sequence
    lcd_send_nibble(0x3); // Function set in 8-bit mode
    sleep_ms(5);
    lcd_send_nibble(0x3);
    sleep_us(150);
    lcd_send_nibble(0x3);
    sleep_us(50);
    lcd_send_nibble(0x2); // Switch to 4-bit mode

    // Function set: 4-bit mode, 2 lines, 5x8 font
    lcd_send_byte(0, 0x28);
    // Display ON/OFF control: Display ON, Cursor OFF, Blink OFF
    lcd_send_byte(0, 0x0C);
    // Clear display
    lcd_send_byte(0, 0x01);
    sleep_ms(2);
    // Entry mode set: Increment cursor, No shift
    lcd_send_byte(0, 0x06);
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
    uint8_t address = (row == 0 ? 0x00 : 0x40) + col;
    lcd_send_byte(0, 0x80 | address); // Set DDRAM address
}

void lcd_print(const char *str) {
    while (*str) {
        lcd_send_byte(1, *str++);
    }
}

void lcd_create_char(uint8_t location, uint8_t *bitmap) {
    location &= 0x7;  // Only 8 locations (0-7)
    lcd_send_byte(0, 0x40 | (location << 3));  // Set CGRAM address
    for (int i = 0; i < 8; i++) {
        lcd_send_byte(1, bitmap[i]);
    }
}