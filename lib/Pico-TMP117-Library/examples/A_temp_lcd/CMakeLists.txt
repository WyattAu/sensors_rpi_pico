# Example A (tmp117/examples/A_temp_lcd/CMakeLists.txt)
add_executable(temp_lcd temp_lcd.c)

# pull in common dependencies and additional i2c hardware support
target_link_libraries(temp_lcd PRIVATE tmp117_functions lcd_functions pico_stdlib pico_printf hardware_i2c)

# add include paths
target_include_directories(temp_lcd PRIVATE ${PROJECT_SOURCE_DIR}/src)

# enable usb output, disable uart output
pico_enable_stdio_usb(temp_lcd 1)
pico_enable_stdio_uart(temp_lcd 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(temp_lcd)

# add url via pico_set_program_url
# example_auto_set_url(temp_lcd)

message(STATUS "END tmp117/examples/A_temp_lcd/CMakeLists.txt")