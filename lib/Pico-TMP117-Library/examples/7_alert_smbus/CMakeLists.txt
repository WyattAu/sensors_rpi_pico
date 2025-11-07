add_executable(alert_smbus alert_smbus.c)

# pull in common dependencies and additional i2c hardware support
target_link_libraries(alert_smbus PRIVATE tmp117_functions pico_stdlib hardware_i2c pico_printf)

# add include paths
target_include_directories(alert_smbus PRIVATE ${PROJECT_SOURCE_DIR}/src)

# enable usb output, disable uart output
pico_enable_stdio_usb(alert_smbus 1)
pico_enable_stdio_uart(alert_smbus 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(alert_smbus)

# add url via pico_set_program_url
# example_auto_set_url(alert_smbus)

message(STATUS "END tmp117/examples/7_alert_smbus/CMakeLists.txt")
