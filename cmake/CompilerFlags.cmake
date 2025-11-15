# Compiler flags for security hardening and reproducible builds

function(set_security_flags target)
    if(MSVC)
        # MSVC with clang-cl
        target_compile_options(${target} PRIVATE
            $<$<COMPILE_LANGUAGE:C,CXX>:/W4>
            $<$<COMPILE_LANGUAGE:C,CXX>:/WX>
            $<$<COMPILE_LANGUAGE:C,CXX>:/guard:cf>
            $<$<COMPILE_LANGUAGE:C,CXX>:/guard:ehcont>
            $<$<COMPILE_LANGUAGE:C,CXX>:/CETCOMPAT>
            $<$<COMPILE_LANGUAGE:C,CXX>:/sdl>
            $<$<COMPILE_LANGUAGE:C,CXX>:/Qspectre>
        )
        target_link_options(${target} PRIVATE
            /guard:cf
            /CETCOMPAT
        )
    elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
        # Clang (Linux, MSYS2, macOS) - disable -Werror due to Pico SDK using GNU extensions
        target_compile_options(${target} PRIVATE
            $<$<COMPILE_LANGUAGE:C,CXX>:-Wall>
            $<$<COMPILE_LANGUAGE:C,CXX>:-Wextra>
            $<$<COMPILE_LANGUAGE:C,CXX>:-Wno-gnu-statement-expression>
            $<$<COMPILE_LANGUAGE:C,CXX>:-Wno-gnu-statement-expression-from-macro-expansion>
            $<$<COMPILE_LANGUAGE:C,CXX>:-O2>
            $<$<COMPILE_LANGUAGE:C,CXX>:-g>
            $<$<COMPILE_LANGUAGE:C,CXX>:-fdebug-prefix-map=${CMAKE_SOURCE_DIR}=/>
        )
        # Disable -Wpedantic for Pico SDK builds due to GNU extensions
        if(NOT CMAKE_SYSTEM_NAME STREQUAL "PICO")
            target_compile_options(${target} PRIVATE
                $<$<COMPILE_LANGUAGE:C,CXX>:-Wpedantic>
            )
        endif()
        # Skip hardening flags for embedded ARM targets
        if(NOT CMAKE_SYSTEM_NAME STREQUAL "PICO")
            target_compile_options(${target} PRIVATE
                $<$<COMPILE_LANGUAGE:C,CXX>:-fPIE>
                $<$<COMPILE_LANGUAGE:C,CXX>:-fPIC>
                $<$<COMPILE_LANGUAGE:C,CXX>:-D_FORTIFY_SOURCE=2>
            )
            target_link_options(${target} PRIVATE
                -Wl,-z,relro
                -Wl,-z,now
                -Wl,-z,noexecstack
                -pie
            )
        endif()
    elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        # GCC (Linux, MSYS2) - disable unsupported security flags for ARM targets
        target_compile_options(${target} PRIVATE
            $<$<COMPILE_LANGUAGE:C,CXX>:-Wall>
            $<$<COMPILE_LANGUAGE:C,CXX>:-Wextra>
            $<$<COMPILE_LANGUAGE:C,CXX>:-O2>
            $<$<COMPILE_LANGUAGE:C,CXX>:-g>
            $<$<COMPILE_LANGUAGE:C,CXX>:-fdebug-prefix-map=${CMAKE_SOURCE_DIR}=/>
        )

        # Check if this is an ARM embedded target
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "cortex|cortex-m" OR CMAKE_C_COMPILER_TARGET MATCHES "arm-none-eabi" OR CMAKE_SYSTEM_NAME STREQUAL "PICO")
            # ARM Cortex-M targets - disable all security hardening flags
            target_compile_options(${target} PRIVATE
                $<$<COMPILE_LANGUAGE:C,CXX>:-fno-stack-protector>
                $<$<COMPILE_LANGUAGE:C,CXX>:-fno-stack-clash-protection>
                $<$<COMPILE_LANGUAGE:C,CXX>:-fcf-protection=none>
                $<$<COMPILE_LANGUAGE:C,CXX>:-fno-PIE>
                $<$<COMPILE_LANGUAGE:C,CXX>:-fno-PIC>
                $<$<COMPILE_LANGUAGE:C,CXX>:-U_FORTIFY_SOURCE>
            )
        else()
            # Desktop targets - enable security hardening
            target_compile_options(${target} PRIVATE
                $<$<COMPILE_LANGUAGE:C,CXX>:-Werror>
                $<$<COMPILE_LANGUAGE:C,CXX>:-Wpedantic>
                $<$<COMPILE_LANGUAGE:C,CXX>:-fstack-protector-strong>
                $<$<COMPILE_LANGUAGE:C,CXX>:-fstack-clash-protection>
                $<$<COMPILE_LANGUAGE:C,CXX>:-fcf-protection=full>
                $<$<COMPILE_LANGUAGE:C,CXX>:-fPIE>
                $<$<COMPILE_LANGUAGE:C,CXX>:-fPIC>
                $<$<COMPILE_LANGUAGE:C,CXX>:-D_FORTIFY_SOURCE=2>
            )
            target_link_options(${target} PRIVATE
                -Wl,-z,relro
                -Wl,-z,now
                -Wl,-z,noexecstack
                -pie
            )
        endif()
    endif()
endfunction()

function(set_reproducible_flags target)
    # Set deterministic build flags
    target_compile_options(${target} PRIVATE
        $<$<COMPILE_LANGUAGE:C,CXX>:-frandom-seed=${CMAKE_PROJECT_NAME}>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wno-builtin-macro-redefined>
        $<$<COMPILE_LANGUAGE:C,CXX>:-D__DATE__="redacted">
        $<$<COMPILE_LANGUAGE:C,CXX>:-D__TIME__="redacted">
        $<$<COMPILE_LANGUAGE:C,CXX>:-D__TIMESTAMP__="redacted">
    )

    # Ensure deterministic linking
    if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
        target_link_options(${target} PRIVATE
            -Wl,--build-id=none
            -Wl,--no-insert-timestamp
        )
    endif()
endfunction()