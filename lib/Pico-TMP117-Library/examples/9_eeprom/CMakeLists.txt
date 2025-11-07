add_executable(eeprom eeprom.c)

# pull in common dependencies and additional i2c hardware support
target_link_libraries(eeprom PRIVATE tmp117_functions
    hardware_i2c
    pico_printf
    pico_stdlib
)

# add include paths
target_include_directories(eeprom PRIVATE ${PROJECT_SOURCE_DIR}/src)

# enable usb output, disable uart output
pico_enable_stdio_usb(eeprom 1)
pico_enable_stdio_uart(eeprom 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(eeprom)

# add url via pico_set_program_url
# example_auto_set_url(eeprom)

message(STATUS "END tmp117/examples/9_eeprom/CMakeLists.txt")
