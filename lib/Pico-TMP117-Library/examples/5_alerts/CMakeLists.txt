# Add the alerts executable
add_executable(alerts alerts.c)

# Pull in common dependencies and additional I2C hardware support for alerts
target_link_libraries(alerts PRIVATE tmp117_functions pico_stdlib hardware_i2c pico_printf)

# Add include paths for alerts
target_include_directories(alerts PRIVATE ${PROJECT_SOURCE_DIR}/src)

# Enable USB output, disable UART output (optional)
pico_enable_stdio_usb(alerts 1)
pico_enable_stdio_uart(alerts 0)

# Create map/bin/hex file for alerts
pico_add_extra_outputs(alerts)

# Add the set_alert_mode_temp_limits executable
add_executable(set_alert_mode_temp_limits set_alert_mode_temp_limits.c)

# Link common libraries to set_alert_mode_temp_limits
target_link_libraries(set_alert_mode_temp_limits PRIVATE tmp117_functions pico_stdlib hardware_i2c pico_printf)

# Add include paths for set_alert_mode_temp_limits
target_include_directories(set_alert_mode_temp_limits PRIVATE ${PROJECT_SOURCE_DIR}/src)

# Enable USB output, disable UART output for set_alert_mode_temp_limits (optional)
pico_enable_stdio_usb(set_alert_mode_temp_limits 1)
pico_enable_stdio_uart(set_alert_mode_temp_limits 0)

# Create map/bin/hex file for set_alert_mode_temp_limits
pico_add_extra_outputs(set_alert_mode_temp_limits)

# add url via pico_set_program_url
# example_auto_set_url(alerts)

message(STATUS "END tmp117/examples/5_alerts/CMakeLists.txt")
