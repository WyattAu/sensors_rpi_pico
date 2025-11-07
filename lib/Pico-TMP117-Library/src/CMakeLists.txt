# CMakeLists.txt in the tmp117/src directory

# Define an INTERFACE library named 'tmp117_functions'
add_library(tmp117_functions INTERFACE)
# Define an INTERFACE library named 'lcd_functions'
add_library(lcd_functions INTERFACE)

# Add the source file tmp117.c to the tmp117_functions interface
target_sources(tmp117_functions INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/tmp117.c
)
# Add the source file lcd.c to the lcd_functions interface
target_sources(lcd_functions INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/lcd.c
)

# Add the current source directory to the include path for tmp117_functions
target_include_directories(tmp117_functions INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
# Add the current source directory to the include path for lcd_functions
target_include_directories(lcd_functions INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Link necessary libraries to the tmp117_functions interface
target_link_libraries(tmp117_functions INTERFACE
    pico_stdlib
    hardware_i2c
    pico_printf
)
# Link necessary libraries to the lcd_functions interface
target_link_libraries(lcd_functions INTERFACE
    pico_stdlib
)

message(STATUS "END tmp117/src/CMakeLists.txt")
