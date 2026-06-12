# Copyright © 2025 CCP ehf.
#[[
Defines our supported build configurations, also known as build flavors.

This compliments the use of toolchain files, as only Carbon components should be configured with these extra types.
Options placed in the toolchain files apply to both this top level project and it's dependencies, whether they be carbon-components or not.

While it is desierable for all dependencies to share the same build flags and other settings provided by the toolchain,
we do not want 3rd party dependencies to be configured with the settings provided by this file.
]]
# this applies to multi-configuration build system, e.g. XCode, Visual Studio, Ninja Multi-Config

set(CMAKE_CONFIGURATION_TYPES Debug TrinityDev Internal Release)

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(is_multi_config)
    set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}"
        CACHE
        STRING
        "Reset the configurations to what we need"
        FORCE
        )
else()
    if (NOT CMAKE_BUILD_TYPE)
        message(STATUS "No CMAKE_BUILD_TYPE was specified, defaulting to 'Debug'")
        set(CMAKE_BUILD_TYPE "Debug")
    endif()
    # Update the documentation string of CMAKE_BUILD_TYPE for GUIs
    set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}"
        CACHE
        STRING
        "Choose the type of build, options are: ${CMAKE_CONFIGURATION_TYPES}."
        FORCE
        )

    # check if a valid build type was supplied
    if (CMAKE_BUILD_TYPE IN_LIST CMAKE_CONFIGURATION_TYPES)
        message(STATUS "Building configuration ${CMAKE_BUILD_TYPE}")
    else()
        message(FATAL_ERROR "Invalid configuration ${CMAKE_BUILD_TYPE}. Available options are: ${CMAKE_CONFIGURATION_TYPES}")
    endif()
endif()

# Create a new configuration based on an existing one
function(create_new_build_config config prototype)
    message(STATUS "Creating new build config '${config}' based on '${prototype}'")

    string(TOUPPER ${config} CONFIG)
    string(TOUPPER ${prototype} PROTOTYPE)

    set(CMAKE_CXX_FLAGS_${CONFIG}
        ${CMAKE_CXX_FLAGS_${PROTOTYPE}} CACHE STRING
        "Flags used by the C++ compiler during ${config} builds."
        FORCE)
    set(CMAKE_C_FLAGS_${CONFIG}
        ${CMAKE_C_FLAGS_${PROTOTYPE}} CACHE STRING
        "Flags used by the C compiler during ${config} builds."
        FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_${CONFIG}
        ${CMAKE_EXE_LINKER_FLAGS_${PROTOTYPE}} CACHE STRING
        "Flags used for linking binaries during ${config} builds."
        FORCE)
    set(CMAKE_MODULE_LINKER_FLAGS_${CONFIG}
        ${CMAKE_MODULE_LINKER_FLAGS_${PROTOTYPE}} CACHE STRING
        "Flags used for linking modules during ${config} builds."
        FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS_${CONFIG}
        ${CMAKE_SHARED_LINKER_FLAGS_${PROTOTYPE}} CACHE STRING
        "Flags used by the shared libraries linker during ${config} builds."
        FORCE)
    mark_as_advanced(
        CMAKE_CXX_FLAGS_${CONFIG}
        CMAKE_C_FLAGS_${CONFIG}
        CMAKE_EXE_LINKER_FLAGS_${CONFIG}
        CMAKE_SHARED_LINKER_FLAGS_${CONFIG})
endfunction()

create_new_build_config(Internal Release)
create_new_build_config(TrinityDev Internal)

function(add_trinity_dev_debug_flags target)
    if (MSVC)
        # Set debug options
        get_target_property(options ${target} COMPILE_OPTIONS)
        string(REGEX REPLACE "<CONFIG:TrinityDev>,/Zi,>" "<CONFIG:TrinityDev>,/ZI,>" options "${options}")
        string(REGEX REPLACE "<CONFIG:TrinityDev>,/O2,>" "<CONFIG:TrinityDev>,/Od,>" options "${options}")
        set_target_properties(${target} PROPERTIES COMPILE_OPTIONS "${options}")
        # Disable /GL and /LTCG for /ZI support
        set_target_properties(${target} PROPERTIES INTERPROCEDURAL_OPTIMIZATION_TRINITYDEV OFF)
        target_link_options(${target} PRIVATE "$<IF:$<CONFIG:TrinityDev>,/LTCG:OFF,>")
    elseif(APPLE)
        target_compile_options(${target} PRIVATE "$<IF:$<CONFIG:TrinityDev>,-Og,>")
    endif()
endfunction()

if (MSVC)
    # https://docs.microsoft.com/en-us/cpp/build/reference/z7-zi-zi-debug-information-format?view=msvc-150
    add_compile_options($<IF:$<OR:$<CONFIG:Release>,$<CONFIG:Internal>>,/Zi,>)
    # Generate Debug Info
    add_link_options($<IF:$<CONFIG:Release>,/DEBUG:FULL,/DEBUG:FASTLINK>)

    # Take manual control over these flags: https://gitlab.kitware.com/cmake/cmake/-/issues/19084
    set(CMAKE_CXX_FLAGS_DEBUG "")
    set(CMAKE_CXX_FLAGS_TRINITYDEV "")
    set(CMAKE_C_FLAGS_TRINITYDEV "")
    set(CMAKE_SHARED_LINKER_FLAGS_TRINITYDEV "")
    set(CMAKE_EXE_LINKER_FLAGS_TRINITYDEV "")

    add_compile_options($<IF:$<CONFIG:Debug>,/ZI,>)
    add_compile_options($<IF:$<CONFIG:Debug>,/Od,>)

    # Default flags which we can override
    add_compile_options($<IF:$<CONFIG:TrinityDev>,/Zi,>)
    add_compile_options($<IF:$<CONFIG:TrinityDev>,/O2,>)
endif()
