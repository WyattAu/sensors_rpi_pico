add_executable(set_offset set_offset.c)

# pull in common dependencies and additional i2c hardware support
target_link_libraries(set_offset PRIVATE tmp117_functions pico_stdlib hardware_watchdog hardware_i2c pico_printf)

# add include paths
target_include_directories(set_offset PRIVATE ${PROJECT_SOURCE_DIR}/src)

# enable usb output, disable uart output
pico_enable_stdio_usb(set_offset 1)
pico_enable_stdio_uart(set_offset 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(set_offset)

# add url via pico_set_program_url
# example_auto_set_url(set_offset)

message(STATUS "END tmp117/examples/3_set_offset/CMakeLists.txt")
