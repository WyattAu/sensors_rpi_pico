# Example 2 (tmp117/examples/2_temp_result_os/CMakeLists.txt)
add_executable(temp_result_os temp_result_os.c)
add_executable(temp_result_os_interrupt temp_result_os_interrupt.c)

# pull in common dependencies and additional i2c hardware support
target_link_libraries(temp_result_os PRIVATE tmp117_functions pico_stdlib pico_printf hardware_i2c)
target_link_libraries(temp_result_os_interrupt PRIVATE tmp117_functions pico_stdlib pico_printf hardware_i2c)

# add include paths
target_include_directories(temp_result_os PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_include_directories(temp_result_os_interrupt PRIVATE ${PROJECT_SOURCE_DIR}/src)

# enable usb output, disable uart output
pico_enable_stdio_usb(temp_result_os 1)
pico_enable_stdio_uart(temp_result_os 0)

# enable usb output, disable uart output
pico_enable_stdio_usb(temp_result_os_interrupt 1)
pico_enable_stdio_uart(temp_result_os_interrupt 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(temp_result_os)
pico_add_extra_outputs(temp_result_os_interrupt)

# add url via pico_set_program_url
# example_auto_set_url(temp_result_os)
message(STATUS "END tmp117/examples/2_temp_result_os/CMakeLists.txt")