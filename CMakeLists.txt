cmake_minimum_required(VERSION 3.16)

cmake_policy(SET CMP0111 NEW)

# Feature options
option(BUILD_SHADER_COMPILER "Build shader compiler." OFF)
if(WIN32)
    option(BUILD_DX11 "Build dx11." OFF)
    option(BUILD_DX12 "Build dx12." OFF)
elseif(APPLE)
    option(BUILD_METAL "Build Metal." OFF)
endif()

if (BUILD_DX11)
    list(APPEND VCPKG_MANIFEST_FEATURES "dx11")
endif()

if (BUILD_DX12)
    list(APPEND VCPKG_MANIFEST_FEATURES "dx12")
endif()

if (BUILD_METAL)
    list(APPEND VCPKG_MANIFEST_FEATURES "metal")
endif()

if (BUILD_SHADER_COMPILER)
    list(APPEND VCPKG_MANIFEST_FEATURES "shader-compiler")
endif()

project(carbon-trinity VERSION 1.0.0)

include(cmake/CcpTargetConfigurations.cmake)
include(cmake/CcpDocsGenerator.cmake)
include(cmake/CcpPackageConfigHelpers.cmake)
include(cmake/CcpBuildConfigurations.cmake)

function(ccp_enable_vs_analizers target)
    set_target_properties(${target} PROPERTIES
        VS_GLOBAL_RunCodeAnalysis false
        VS_GLOBAL_EnableMicrosoftCodeAnalysis false
        VS_GLOBAL_EnableClangTidyCodeAnalysis true
    )
endfunction()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
#set(CMAKE_COMPILE_WARNING_AS_ERROR ON) TODO turned off for testing

if (BUILD_SHADER_COMPILER)
    add_subdirectory(shadercompiler)
endif ()

add_subdirectory(trinityal)
add_subdirectory(trinity)
