add_executable(nist nist.c)

# pull in common dependencies and additional i2c hardware support
target_link_libraries(nist PRIVATE tmp117_functions
    hardware_gpio
    hardware_i2c
    pico_printf
    pico_stdio
    )

# add include paths
target_include_directories(nist PRIVATE ${PROJECT_SOURCE_DIR}/src)

# enable usb output, disable uart output
pico_enable_stdio_usb(nist 1)
pico_enable_stdio_uart(nist 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(nist)

# add url via pico_set_program_url
# example_auto_set_url(nist)

message(STATUS "END tmp117/examples/8_NIST-ID/CMakeLists.txt")
