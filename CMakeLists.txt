# Modern CMake project for Raspberry Pi Pico sensors
cmake_minimum_required(VERSION 3.20)

# Set policies for modern CMake behavior
cmake_policy(SET CMP0077 NEW)  # Allow overriding options
cmake_policy(SET CMP0091 NEW)  # Enable MSVC_RUNTIME_LIBRARY selection

# Enable reproducible builds by setting SOURCE_DATE_EPOCH if not set
if(NOT DEFINED ENV{SOURCE_DATE_EPOCH})
    string(TIMESTAMP SOURCE_DATE_EPOCH "%s" UTC)
    set(ENV{SOURCE_DATE_EPOCH} ${SOURCE_DATE_EPOCH})
endif()

# Set C and C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set default build type to Release for reproducible builds
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Option to build tests (host-based unit tests)
option(BUILD_TESTS "Build unit tests" OFF)

# For host testing, use generic system to avoid cross-compilation setup
if(BUILD_TESTS)
    set(CMAKE_SYSTEM_NAME Generic)
endif()

# Include CPM.cmake for dependency management
include(CPM.cmake)

# Set PICO_BOARD before including Pico SDK
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Note: Clang builds are currently not functional due to Pico SDK inline assembly compatibility issues
# Use GCC-based presets (msys2-gcc, linux-gcc) for successful builds

# Download and extract ARM GNU Toolchain if needed (only for cross-compilation)
if(NOT USE_CLANG AND NOT CMAKE_TOOLCHAIN_FILE AND NOT BUILD_TESTS)
    set(TOOLCHAIN_VERSION "14.3.rel1")

    # Detect host platform and set appropriate toolchain
    if(WIN32)
        set(TOOLCHAIN_URL "https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu/${TOOLCHAIN_VERSION}/binrel/arm-gnu-toolchain-${TOOLCHAIN_VERSION}-mingw-w64-i686-arm-none-eabi.zip")
        set(TOOLCHAIN_SHA256 "836ebe51fd71b6542dd7884c8fb2011192464b16c28e4b38fddc9350daba5ee8")
        set(TOOLCHAIN_ARCHIVE_NAME "arm-gnu-toolchain.zip")
        set(TOOLCHAIN_EXTRACT_DIR_NAME "arm-gnu-toolchain-${TOOLCHAIN_VERSION}-mingw-w64-i686-arm-none-eabi")
    else()
        set(TOOLCHAIN_URL "https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu/${TOOLCHAIN_VERSION}/binrel/arm-gnu-toolchain-${TOOLCHAIN_VERSION}-x86_64-arm-none-eabi.tar.xz")
        set(TOOLCHAIN_SHA256 "8f6903f8ceb084d9227b9ef991490413014d991874a1e34074443c2a72b14dbd")
        set(TOOLCHAIN_ARCHIVE_NAME "arm-gnu-toolchain.tar.xz")
        set(TOOLCHAIN_EXTRACT_DIR_NAME "arm-gnu-toolchain-${TOOLCHAIN_VERSION}-x86_64-arm-none-eabi")
    endif()

    # Set toolchain directory
    set(TOOLCHAIN_DIR "${CMAKE_BINARY_DIR}/toolchain")
    set(TOOLCHAIN_ARCHIVE "${TOOLCHAIN_DIR}/${TOOLCHAIN_ARCHIVE_NAME}")
    set(TOOLCHAIN_EXTRACT_DIR "${TOOLCHAIN_DIR}/${TOOLCHAIN_EXTRACT_DIR_NAME}")

    # Download toolchain if not exists
    if(NOT EXISTS "${TOOLCHAIN_EXTRACT_DIR}")
        message(STATUS "Downloading ARM GNU Toolchain ${TOOLCHAIN_VERSION}...")
        file(DOWNLOAD "${TOOLCHAIN_URL}" "${TOOLCHAIN_ARCHIVE}"
              EXPECTED_HASH SHA256=${TOOLCHAIN_SHA256}
              TLS_VERIFY ON
              SHOW_PROGRESS)
        message(STATUS "Extracting ARM GNU Toolchain...")
        file(ARCHIVE_EXTRACT INPUT "${TOOLCHAIN_ARCHIVE}" DESTINATION "${TOOLCHAIN_DIR}")
    endif()

    # Set PATH to include toolchain binaries
    set(ENV{PATH} "${TOOLCHAIN_EXTRACT_DIR}/bin;$ENV{PATH}")
endif()

# Pull in Raspberry Pi Pico SDK using CPM
CPMAddPackage(
    NAME pico-sdk
    GITHUB_REPOSITORY raspberrypi/pico-sdk
    GIT_TAG 2.2.0
    DOWNLOAD_ONLY YES
)

# Set PICO_SDK_PATH for dependencies
set(PICO_SDK_PATH ${pico-sdk_SOURCE_DIR})

# Initialise the Raspberry Pi Pico SDK (only for cross-compilation)
if(NOT BUILD_TESTS)
    # Include the Pico SDK init cmake file
    include(${PICO_SDK_PATH}/pico_sdk_init.cmake)
endif()

# Set the toolchain file for ARM cross-compilation (only for cross-compilation)
# CMAKE_TOOLCHAIN_FILE must be set before the first project() call
if(NOT CMAKE_TOOLCHAIN_FILE AND NOT BUILD_TESTS AND NOT USE_CUSTOM_TOOLCHAIN)
    if(USE_CLANG)
        set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/build/release/msys2-clang/toolchain.cmake)
        # Set default Clang runtime for embedded ARM
        if(NOT PICO_CLANG_RUNTIMES)
            set(PICO_CLANG_RUNTIMES "arm-none-eabi")
        endif()
    else()
        set(CMAKE_TOOLCHAIN_FILE ${PICO_SDK_PATH}/cmake/preload/toolchains/pico_arm_cortex_m33_gcc.cmake)
    endif()
endif()

project(sensors_rpi_pico
    VERSION 0.1.0
    DESCRIPTION "Raspberry Pi Pico sensors project"
    LANGUAGES C CXX ASM
)

# Override CMAKE_SYSTEM_NAME to prevent security flags from being applied (before pico_sdk_init)
if(NOT BUILD_TESTS)
    set(CMAKE_SYSTEM_NAME PICO)
    # Disable CMake's default security hardening flags for Release builds
    set(CMAKE_C_FLAGS_RELEASE "-fno-stack-protector -fno-stack-clash-protection -fcf-protection=none -fno-PIE -fno-PIC -U_FORTIFY_SOURCE")
    set(CMAKE_CXX_FLAGS_RELEASE "-fno-stack-protector -fno-stack-clash-protection -fcf-protection=none -fno-PIE -fno-PIC -U_FORTIFY_SOURCE")
    # Also disable for all build types
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-stack-protector -fno-stack-clash-protection -fcf-protection=none -fno-PIE -fno-PIC -U_FORTIFY_SOURCE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-stack-protector -fno-stack-clash-protection -fcf-protection=none -fno-PIE -fno-PIC -U_FORTIFY_SOURCE")
    # Force disable security flags globally
    add_compile_options(-fno-stack-protector -fno-stack-clash-protection -fcf-protection=none -fno-PIE -fno-PIC -U_FORTIFY_SOURCE)
endif()

# Initialize the Raspberry Pi Pico SDK (only for cross-compilation)
if(NOT BUILD_TESTS)
    pico_sdk_init()

    # Security flags are now set in the toolchain.cmake file
endif()

# Enable testing if BUILD_TESTS is ON (must be after project() call)
if(BUILD_TESTS)
    enable_testing()
    include(CTest)
endif()

# Pull in Pimoroni Pico libraries using CPM
CPMAddPackage(
    NAME pimoroni-pico
    GITHUB_REPOSITORY pimoroni/pimoroni-pico
    GIT_TAG main
    DOWNLOAD_ONLY YES
)


# Add executable with modern target-based approach (only for cross-compilation)
if(NOT BUILD_TESTS)
    add_executable(sensors_rpi_pico)

    target_sources(sensors_rpi_pico PRIVATE
        target/standalone/src/main.c
        target/standalone/src/tmp117.c
        target/standalone/src/cmps12.c
    )

    # Set language property for main.c to compile as C++
    set_source_files_properties(target/standalone/src/main.c PROPERTIES LANGUAGE CXX)

    # Set target properties
    pico_set_program_name(sensors_rpi_pico "sensors_rpi_pico")
    pico_set_program_version(sensors_rpi_pico "${PROJECT_VERSION}")

    # Configure stdio
    pico_enable_stdio_uart(sensors_rpi_pico 0)
    pico_enable_stdio_usb(sensors_rpi_pico 1)

    # Add include directories
    target_include_directories(sensors_rpi_pico PRIVATE
        target/standalone/src
        lib
    )

    # Set compile features
    target_compile_features(sensors_rpi_pico PRIVATE
        c_std_11
        cxx_std_17
    )

    # Security hardening and reproducible build flags (skip for embedded ARM targets)
    # For PICO builds, flags are already set in the toolchain.cmake file
    # We skip including CompilerFlags.cmake to avoid conflicts with toolchain flags

    # Link libraries
    target_link_libraries(sensors_rpi_pico PRIVATE
        pico_stdlib
        hardware_i2c
    )

    # Add extra outputs
    pico_add_extra_outputs(sensors_rpi_pico)

    # Install target (optional)
    install(TARGETS sensors_rpi_pico
        RUNTIME DESTINATION bin
    )
endif()

# Testing infrastructure
if(BUILD_TESTS)
    # Fetch GTest using CPM
    CPMAddPackage(
        NAME googletest
        GITHUB_REPOSITORY google/googletest
        VERSION 1.14.0
    )

    # Create test executable
    add_executable(sensors_tests)

    target_sources(sensors_tests PRIVATE
        tests/main.cpp
        tests/test_tmp117.cpp
        tests/test_cmps12.cpp
        tests/test_cpm_libraries.cpp
        tests/test_sensors_integration.cpp
    )

    # Set test target properties
    target_compile_features(sensors_tests PRIVATE
        c_std_11
        cxx_std_17
    )

    # Include directories for tests
    target_include_directories(sensors_tests PRIVATE
        target/standalone/src
        lib/Pico-TMP117-Library/src
        lib/Pico-TMP117-Library
        ${pico-sdk_SOURCE_DIR}
    )

    # Define HOST_TESTING for conditional compilation
    target_compile_definitions(sensors_tests PRIVATE HOST_TESTING)

    # Link GTest libraries
    target_link_libraries(sensors_tests PRIVATE
        GTest::gtest
        GTest::gtest_main
    )

    # Add test to CTest
    add_test(NAME sensors_unit_tests COMMAND sensors_tests)

    # Set test properties
    set_tests_properties(sensors_unit_tests PROPERTIES
        TIMEOUT 60
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    # Coverage configuration for Windows (OpenCppCoverage)
    if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "MSVC|Clang")
        find_program(OPENCPPCOVERAGE_EXECUTABLE opencppcoverage)
        if(OPENCPPCOVERAGE_EXECUTABLE)
            add_custom_target(coverage
                COMMAND ${OPENCPPCOVERAGE_EXECUTABLE}
                    --sources ${CMAKE_SOURCE_DIR}
                    --excluded_sources ${CMAKE_SOURCE_DIR}/tests
                    --excluded_sources ${CMAKE_SOURCE_DIR}/build
                    --excluded_sources ${CMAKE_SOURCE_DIR}/lib
                    -- ${CMAKE_BINARY_DIR}/sensors_tests.exe
                    --gtest_output=xml:${CMAKE_BINARY_DIR}/coverage.xml
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Running tests with coverage analysis"
                VERBATIM
            )
        endif()
    endif()
endif()

