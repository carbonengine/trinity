# Copyright © 2025 CCP ehf.
#[===[
Note: Use of this file is deprecated, but is being kept around for backwards compatability purposes. It will be deleted soon

Helpers functions for creating config files that can be included by the EVE monolith.

Adds the `configure_ccp_vendor_config_file` command.

## configure_ccp_vendor_config_file

Creates a config file for a project that respects EVE's vendoring rules.

```
configure_ccp_vendor_config_file(
    TARGET <target>
    DESTINATION <path>
    [DEPENDENCIES <dep1> <dep2> ... <depN>]
    [EXPORT_DLL]
)
```

`configure_ccp_vendor_config_file()` should be used instead of CMake's `configure_package_config_file()` or `configure_file()`
commands when creating the `<PackageName>Config.cmake` file for installing a CCP project or library. It helps making the
resulting package adhere to CCP's vendor folder structure.

]===]
function(configure_ccp_vendor_config_file)
    cmake_parse_arguments(CCP_PACKAGE_CONFIG "EXPORT_DLL" "TARGET;DESTINATION" "DEPENDENCIES;CONFIGURATIONS" ${ARGN})
    set(CCP_PACKAGE_CONFIG_TARGET_DEPENDENCIES "")
    foreach(_DEP ${CCP_PACKAGE_CONFIG_DEPENDENCIES})
        set(CCP_PACKAGE_CONFIG_TARGET_DEPENDENCIES "${CCP_PACKAGE_CONFIG_TARGET_DEPENDENCIES}find_dependency(${_DEP} REQUIRED CONFIG NO_CMAKE_PATH)\n")
    endforeach()
    if(NOT CCP_PACKAGE_CONFIG_CONFIGURATIONS)
        set(CCP_PACKAGE_CONFIG_CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES})
    endif()
    get_target_property(CCP_PACKAGE_CONFIG_TARGET_TYPE ${CCP_PACKAGE_CONFIG_TARGET} TYPE)
    if (${CCP_PACKAGE_CONFIG_TARGET_TYPE} STREQUAL STATIC_LIBRARY)
        set(CCP_PACKAGE_CONFIG_TARGET_TYPE STATIC)
    elseif (${CCP_PACKAGE_CONFIG_TARGET_TYPE} STREQUAL SHARED_LIBRARY)
        set(CCP_PACKAGE_CONFIG_TARGET_TYPE SHARED)
    endif()
    get_target_property(CCP_PACKAGE_CONFIG_TARGET_INTERFACE_COMPILE_DEFINITIONS ${CCP_PACKAGE_CONFIG_TARGET} INTERFACE_COMPILE_DEFINITIONS)
    if(NOT CCP_PACKAGE_CONFIG_TARGET_INTERFACE_COMPILE_DEFINITIONS)
        # Avoid defining CCP_PACKAGE_CONFIG_TARGET_INTERFACE_COMPILE_DEFINITIONS-NOTFOUND 1
        set(CCP_PACKAGE_CONFIG_TARGET_INTERFACE_COMPILE_DEFINITIONS "")
    endif()
    get_target_property(CCP_PACKAGE_CONFIG_TARGET_INTERFACE_LINK_LIBRARIES ${CCP_PACKAGE_CONFIG_TARGET} INTERFACE_LINK_LIBRARIES)
    if(NOT CCP_PACKAGE_CONFIG_TARGET_INTERFACE_LINK_LIBRARIES)
        # Avoid defining CCP_PACKAGE_CONFIG_TARGET_INTERFACE_LINK_LIBRARIES-NOTFOUND 1
        set(CCP_PACKAGE_CONFIG_TARGET_INTERFACE_LINK_LIBRARIES "")
    endif()

    # CMake falls back to the logical name if an output_name was not specified - we usually do not specify an output name
    # either, but let's deal with that just in case
    get_target_property(CCP_PACKAGE_CONFIG_TARGET_FILE_NAME ${CCP_PACKAGE_CONFIG_TARGET} OUTPUT_NAME)
    if(NOT CCP_PACKAGE_CONFIG_TARGET_FILE_NAME)
        get_target_property(CCP_PACKAGE_CONFIG_TARGET_FILE_NAME ${CCP_PACKAGE_CONFIG_TARGET} NAME)
    endif()

    # We cannot simply take the target's interface include directories in their pure form because all used variables inside them will be
    # resolved to absolute paths by the time we access the property. In the end, however, we need to turn these back into paths which are
    # relative to the generated configuration script - e.g. CMAKE_CURRENT_LIST_DIR - and that's what we attempt to do here.
    # This is likely not sufficient for more complex use cases, but seems to get us far enough at the moment.
    get_target_property(CCP_PACKAGE_CONFIG_TARGET_INTERFACE_INCLUDE_DIRECTORIES ${CCP_PACKAGE_CONFIG_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
    set(_DIRS "")
    foreach(_DIR ${CCP_PACKAGE_CONFIG_TARGET_INTERFACE_INCLUDE_DIRECTORIES})
        string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR} [=[${CMAKE_CURRENT_LIST_DIR}]=] _DIR ${_DIR})
        list(APPEND _DIRS ${_DIR})
    endforeach()
    set(CCP_PACKAGE_CONFIG_TARGET_INTERFACE_INCLUDE_DIRECTORIES ${_DIRS})

    configure_file(
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templatePackageConfig.cmake.in"
        "${CCP_PACKAGE_CONFIG_DESTINATION}"
        NEWLINE_STYLE UNIX
        @ONLY
    )
endfunction()
