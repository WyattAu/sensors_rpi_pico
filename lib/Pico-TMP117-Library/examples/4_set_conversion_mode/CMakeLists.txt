add_executable(set_conversion_mode set_conversion_mode.c)

# pull in common dependencies and additional i2c hardware support
target_link_libraries(set_conversion_mode PRIVATE tmp117_functions pico_stdlib hardware_i2c pico_printf)

# add include paths
target_include_directories(set_conversion_mode PRIVATE ${PROJECT_SOURCE_DIR}/src)

# enable usb output, disable uart output
pico_enable_stdio_usb(set_conversion_mode 1)
pico_enable_stdio_uart(set_conversion_mode 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(set_conversion_mode)

# add url via pico_set_program_url
# example_auto_set_url(set_conversion_mode)

message(STATUS "END tmp117/examples/4_set_conversion_mode/CMakeLists.txt")
