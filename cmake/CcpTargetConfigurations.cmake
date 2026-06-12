# Copyright © 2025 CCP ehf.
macro(ensure_correct_target_type target)
    get_target_property(target_type ${target} TYPE)
    if(${target_type} STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()
    get_target_property(aliased_target ${target} ALIASED_TARGET)
    if(aliased_target)
        return()
    endif()
    get_target_property(imported_target ${target} IMPORTED)
    if(imported_target)
        return()
    endif()
endmacro()

function(set_build_flavor target)
    ensure_correct_target_type(${target})
    set_target_properties(${target}
        PROPERTIES
            DEBUG_POSTFIX "_debug"
            TRINITYDEV_POSTFIX "_trinitydev"
            INTERNAL_POSTFIX "_internal"
    )
    target_compile_definitions(${target} PRIVATE CCP_BUILD_FLAVOR=$<LOWER_CASE:$<IF:$<CONFIG:Release>,,_$<CONFIG>>>)
endfunction()

function(externalize_apple_debuginfo name)
    if (NOT APPLE)
        return()
    endif()

    if(CMAKE_GENERATOR STREQUAL "Xcode")
        return()
    endif()

    get_target_property(target_type ${name} TYPE)
    if(target_type STREQUAL STATIC_LIBRARY)
        return()
    endif()

    ensure_correct_target_type(${name})

    add_custom_command(TARGET ${name} POST_BUILD
        # Extract symbols to flat dsym file
        VERBATIM COMMAND $<$<CONFIG:Release>:dsymutil> "$<TARGET_FILE:${name}>" -f -o $<TARGET_FILE:${name}>.sym
        # Remove symbols from binary
        VERBATIM COMMAND $<$<CONFIG:Release>:strip> -Sxl $<TARGET_FILE:${name}>
    )
endfunction()

function(set_prefix_and_suffix target)
    # Configure output directories, prefixes and suffixes
    ensure_correct_target_type(${target})
    set_target_properties(${target}
        PROPERTIES
            PREFIX ""
    )
    if(target_type STREQUAL SHARED_LIBRARY OR target_type STREQUAL MODULE_LIBRARY)
        if(APPLE)
            # on macOS we like to still use the .so naming convention, without a prefix
            set_target_properties(${target}
                PROPERTIES
                    SUFFIX ".so"
            )
        elseif(WIN32)
            # Python3 requires suffix to be pyd on Windows
            set_target_properties(${target}
                PROPERTIES
                    SUFFIX ".pyd"
            )
        endif()
    endif()
endfunction()

function(ccp_add_executable)
    # Adds and configures an executable target for CCPs
    # vendored packages. 
    set(target ${ARGV0})
    add_executable(${ARGN})
    set_prefix_and_suffix(${target})
    set_build_flavor(${target})
    externalize_apple_debuginfo(${target})
endfunction()

function(ccp_add_library)
    # Adds and configures a library target for CCPs
    # vendored packages.
    set(target ${ARGV0})
    add_library(${ARGN})
    set_prefix_and_suffix(${target})
    set_build_flavor(${target})
    externalize_apple_debuginfo(${target})
endfunction()
