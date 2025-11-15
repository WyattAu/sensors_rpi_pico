cmake_minimum_required(VERSION 3.20)

cmake_policy(SET CMP0077 NEW)
cmake_policy(SET CMP0091 NEW)

if(NOT DEFINED ENV{SOURCE_DATE_EPOCH})
    string(TIMESTAMP SOURCE_DATE_EPOCH "%s" UTC)
    set(ENV{SOURCE_DATE_EPOCH} ${SOURCE_DATE_EPOCH})
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

option(BUILD_TESTS "Build unit tests" OFF)

if(BUILD_TESTS)
    set(CMAKE_SYSTEM_NAME Generic)
endif()

include(CPM.cmake)

set(PICO_BOARD pico2 CACHE STRING "Board type")

if(NOT USE_CLANG AND NOT CMAKE_TOOLCHAIN_FILE AND NOT BUILD_TESTS)
    set(TOOLCHAIN_VERSION "14.3.rel1")

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

    set(TOOLCHAIN_DIR "${CMAKE_BINARY_DIR}/toolchain")
    set(TOOLCHAIN_ARCHIVE "${TOOLCHAIN_DIR}/${TOOLCHAIN_ARCHIVE_NAME}")
    set(TOOLCHAIN_EXTRACT_DIR "${TOOLCHAIN_DIR}/${TOOLCHAIN_EXTRACT_DIR_NAME}")

    if(NOT EXISTS "${TOOLCHAIN_EXTRACT_DIR}")
        message(STATUS "Downloading ARM GNU Toolchain ${TOOLCHAIN_VERSION}...")
        file(DOWNLOAD "${TOOLCHAIN_URL}" "${TOOLCHAIN_ARCHIVE}"
              EXPECTED_HASH SHA256=${TOOLCHAIN_SHA256}
              TLS_VERIFY ON
              SHOW_PROGRESS)
        message(STATUS "Extracting ARM GNU Toolchain...")
        file(ARCHIVE_EXTRACT INPUT "${TOOLCHAIN_ARCHIVE}" DESTINATION "${TOOLCHAIN_DIR}")
    endif()

    set(ENV{PATH} "${TOOLCHAIN_EXTRACT_DIR}/bin;$ENV{PATH}")
endif()

CPMAddPackage(
    NAME pico-sdk
    GITHUB_REPOSITORY raspberrypi/pico-sdk
    GIT_TAG 2.2.0
    DOWNLOAD_ONLY YES
)

set(PICO_SDK_PATH ${pico-sdk_SOURCE_DIR})

if(NOT BUILD_TESTS)
    include(${PICO_SDK_PATH}/pico_sdk_init.cmake)
endif()

if(NOT CMAKE_TOOLCHAIN_FILE AND NOT BUILD_TESTS AND NOT USE_CUSTOM_TOOLCHAIN)
    if(USE_CLANG)
        set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/build/release/msys2-clang/toolchain.cmake)
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

if(NOT BUILD_TESTS)
    set(CMAKE_SYSTEM_NAME PICO)
    set(CMAKE_C_FLAGS_RELEASE "-fno-stack-protector -fno-stack-clash-protection -fcf-protection=none -fno-PIE -fno-PIC -U_FORTIFY_SOURCE")
    set(CMAKE_CXX_FLAGS_RELEASE "-fno-stack-protector -fno-stack-clash-protection -fcf-protection=none -fno-PIE -fno-PIC -U_FORTIFY_SOURCE")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-stack-protector -fno-stack-clash-protection -fcf-protection=none -fno-PIE -fno-PIC -U_FORTIFY_SOURCE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-stack-protector -fno-stack-clash-protection -fcf-protection=none -fno-PIE -fno-PIC -U_FORTIFY_SOURCE")
    add_compile_options(-fno-stack-protector -fno-stack-clash-protection -fcf-protection=none -fno-PIE -fno-PIC -U_FORTIFY_SOURCE)
endif()

if(NOT BUILD_TESTS)
    pico_sdk_init()
endif()

if(BUILD_TESTS)
    enable_testing()
    include(CTest)
endif()

CPMAddPackage(
    NAME pimoroni-pico
    GITHUB_REPOSITORY pimoroni/pimoroni-pico
    GIT_TAG main
    DOWNLOAD_ONLY YES
)

if(NOT BUILD_TESTS)
    add_executable(sensors_rpi_pico)

    target_sources(sensors_rpi_pico PRIVATE
        target/standalone/src/main.c
        target/standalone/src/tmp117.c
        target/standalone/src/cmps12.c
    )

    set_source_files_properties(target/standalone/src/main.c PROPERTIES LANGUAGE CXX)

    pico_set_program_name(sensors_rpi_pico "sensors_rpi_pico")
    pico_set_program_version(sensors_rpi_pico "${PROJECT_VERSION}")

    pico_enable_stdio_uart(sensors_rpi_pico 0)
    pico_enable_stdio_usb(sensors_rpi_pico 1)

    target_include_directories(sensors_rpi_pico PRIVATE
        target/standalone/src
        lib
    )

    target_compile_features(sensors_rpi_pico PRIVATE
        c_std_11
        cxx_std_17
    )

    target_link_libraries(sensors_rpi_pico PRIVATE
        pico_stdlib
        hardware_i2c
    )

    pico_add_extra_outputs(sensors_rpi_pico)

    install(TARGETS sensors_rpi_pico
        RUNTIME DESTINATION bin
    )
endif()

if(BUILD_TESTS)
    CPMAddPackage(
        NAME googletest
        GITHUB_REPOSITORY google/googletest
        VERSION 1.14.0
    )

    add_executable(sensors_tests)

    target_sources(sensors_tests PRIVATE
        tests/main.cpp
        tests/test_tmp117.cpp
        tests/test_cmps12.cpp
        tests/test_cpm_libraries.cpp
        tests/test_sensors_integration.cpp
    )

    target_compile_features(sensors_tests PRIVATE
        c_std_11
        cxx_std_17
    )

    target_include_directories(sensors_tests PRIVATE
        target/standalone/src
        lib/Pico-TMP117-Library/src
        lib/Pico-TMP117-Library
        ${pico-sdk_SOURCE_DIR}
    )

    target_compile_definitions(sensors_tests PRIVATE HOST_TESTING)

    target_link_libraries(sensors_tests PRIVATE
        GTest::gtest
        GTest::gtest_main
    )

    add_test(NAME sensors_unit_tests COMMAND sensors_tests)

    set_tests_properties(sensors_unit_tests PROPERTIES
        TIMEOUT 60
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

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

