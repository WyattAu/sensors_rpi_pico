// lcd.h
#ifndef LCD_H
#define LCD_H

#include "pico/stdlib.h"
#include <stdint.h>

// Structure to hold pin assignments
typedef struct {
    uint8_t rs_pin; // Register Select pin
    uint8_t enable_pin; // Enable pin
    uint8_t data_pins[4]; // Data pins (D4-D7 for 4-bit mode)
} LCD_Config;

void lcd_pin_init(LCD_Config *config);
void lcd_pulse_enable(void);
void lcd_send_nibble(uint8_t nibble);
void lcd_send_byte(uint8_t rs, uint8_t data);
void lcd_init(void);
void lcd_set_cursor(uint8_t row, uint8_t col);
void lcd_print(const char *str);
void lcd_create_char(uint8_t location, uint8_t *bitmap);

#endif // LCD_H