# Example 1 (tmp117/examples/1_temp_result_cc/CMakeLists.txt)
add_executable(temp_result temp_result.c)
add_executable(temp_time temp_time.c)

# pull in common dependencies and additional i2c hardware support
target_link_libraries(temp_result PRIVATE tmp117_functions pico_stdlib pico_printf hardware_i2c)
target_link_libraries(temp_time PRIVATE tmp117_functions pico_stdlib pico_printf hardware_i2c)

# add include paths
target_include_directories(temp_result PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_include_directories(temp_time PRIVATE ${PROJECT_SOURCE_DIR}/src)

# enable usb output, disable uart output
pico_enable_stdio_usb(temp_result 1)
pico_enable_stdio_uart(temp_result 0)

# enable usb output, disable uart output
pico_enable_stdio_usb(temp_result 1)
pico_enable_stdio_uart(temp_result 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(temp_result)
pico_add_extra_outputs(temp_time)

# add url via pico_set_program_url
# example_auto_set_url(temp_result)
# example_auto_set_url(temp_time)

message(STATUS "END tmp117/examples/1_temp_result_cc/CMakeLists.txt")