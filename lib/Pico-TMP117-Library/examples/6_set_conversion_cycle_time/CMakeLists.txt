add_executable(set_conversion_avg_cycle set_conversion_avg_cycle.c)

# pull in common dependencies and additional i2c hardware support
target_link_libraries(set_conversion_avg_cycle PRIVATE tmp117_functions pico_stdlib pico_printf hardware_i2c hardware_watchdog)

# add include paths
# target_include_directories(set_conversion_avg_cycle.c PRIVATE ${PROJECT_SOURCE_DIR}/src)

# enable usb output, disable uart output
pico_enable_stdio_usb(set_conversion_avg_cycle 1)
pico_enable_stdio_uart(set_conversion_avg_cycle 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(set_conversion_avg_cycle)

# add url via pico_set_program_url
# example_auto_set_url(set_conversion_avg_cycle)

message(STATUS "END tmp117/examples/6_set_conversion_cycle_time/CMakeLists.txt")
