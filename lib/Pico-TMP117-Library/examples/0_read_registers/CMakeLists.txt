# Example 0 (tmp117/examples/0_register_readout/CMakeLists.txt)
add_executable(read_registers read_registers.c)

# pull in common dependencies and additional i2c hardware support
target_link_libraries(read_registers PRIVATE tmp117_functions pico_stdlib hardware_i2c pico_printf)

# add include paths
target_include_directories(read_registers PRIVATE ${PROJECT_SOURCE_DIR}/src)

# enable usb output, disable uart output
pico_enable_stdio_usb(read_registers 1)
pico_enable_stdio_uart(read_registers 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(read_registers)

# add url via pico_set_program_url
# example_auto_set_url(read_registers)

message(STATUS "END tmp117/examples/0_read_registers/CMakeLists.txt")
