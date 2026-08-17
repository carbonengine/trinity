# Copyright © 2021 CCP ehf.

cmake_minimum_required(VERSION 3.16)

cmake_policy(SET CMP0111 NEW)

if(BUILD_FOR_PYTHON_2)

    set(BUILD_DX11 ON)
    set(BUILD_DX12 ON)
    set(BUILD_METAL ON)
    set(BUILD_SHADER_COMPILER ON)
    set(INSTALL_TO_MONOLITH ON)

else()
    # Feature options
    option(BUILD_SHADER_COMPILER "Build shader compiler." OFF)
    if(WIN32)
        option(BUILD_DX11 "Build dx11." OFF)
        option(BUILD_DX12 "Build dx12." OFF)
        # vulkan-headers and vulkan-memory-allocator are core, non-feature
        # dependencies in vcpkg.json, so the backend's own compile surface widens the
        # closure by nothing. The "vulkan" manifest feature carries exactly two ports:
        #
        #   directx-dxc   -- the TrinityALTest_vulkan shader step needs it to emit
        #                    SPIR-V (fxc cannot), and it is otherwise gated behind
        #                    dx11/dx12/shader-compiler, all OFF here.
        #   vulkan-loader -- the import library for the vk* entry points. The headers
        #                    ship no library, which nothing noticed while Phase 1 built
        #                    the backend as a static archive and linked no executable
        #                    against it (85 unresolved vk* symbols; see
        #                    docs/spikes/M4-phase2a-test-harness.md §4a).
        #
        # Both are public ports from the vendored default microsoft/vcpkg registry, so
        # this preserves the property that the Vulkan configuration is the one Windows
        # configuration that builds outside the CCP network -- which is the argument to
        # re-check against this list, not against this comment, before adding a third.
        option(BUILD_VULKAN "Build Vulkan." OFF)
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

    if (BUILD_VULKAN)
        list(APPEND VCPKG_MANIFEST_FEATURES "vulkan")
    endif()

    if (BUILD_SHADER_COMPILER)
        list(APPEND VCPKG_MANIFEST_FEATURES "shader-compiler")
    endif()

    set(WITH_GRANNY_DEFAULT_FLAG OFF)

    option(WITH_GRANNY "Compile with Granny for .gr2 file support" ${WITH_GRANNY_DEFAULT_FLAG})

    if (WITH_GRANNY)
        list(APPEND VCPKG_MANIFEST_FEATURES "with-granny")
    endif()

endif()

project(carbon-trinity VERSION 1.0.0)

# Enables CTest at the scope every add_subdirectory() below inherits from, so a
# ctest run from the build root can see tests registered by any subdirectory
# (currently trinityal/tests/CMakeLists.txt's TrinityALTest_vulkan). This is
# additive only -- it defines no targets and does not touch BUILD_TESTING --
# so it changes nothing about what dx11, dx12, or metal configurations build.
enable_testing()

if (BUILD_FOR_PYTHON_2)
    if(NOT DEFINED ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
        message(FATAL_ERROR "Missing required environment variable CCP_EVE_PERFORCE_BRANCH_PATH")
    elseif(NOT IS_ABSOLUTE $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
        message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH needs to be an absolute path")
    elseif(NOT EXISTS $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
        message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH is pointing at a non-existing location $ENV{CCP_EVE_PERFORCE_BRANCH_PATH}")
    elseif(NOT IS_DIRECTORY $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
        message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH needs to be a directory")
    else()
        message(STATUS "Using CCP_EVE_PERFORCE_BRANCH_PATH located at $ENV{CCP_EVE_PERFORCE_BRANCH_PATH}")
    endif()

    file(TO_CMAKE_PATH "$ENV{CCP_EVE_PERFORCE_BRANCH_PATH}" BRANCH_ROOT_DIR)
    list(INSERT CMAKE_MODULE_PATH 0 ${BRANCH_ROOT_DIR}/cmake)
    include(CcpGlobalSettings)
    include(CcpPackageConfigHelpers)
else()
    include(cmake/CcpTargetConfigurations.cmake)
    include(cmake/CcpDocsGenerator.cmake)
    include(cmake/CcpPackageConfigHelpers.cmake)
    include(cmake/CcpBuildConfigurations.cmake)
endif()

function(ccp_enable_vs_analizers target)
    set_target_properties(${target} PROPERTIES
        VS_GLOBAL_RunCodeAnalysis false
        VS_GLOBAL_EnableMicrosoftCodeAnalysis false
        VS_GLOBAL_EnableClangTidyCodeAnalysis true
    )
endfunction()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(BUILD_FOR_PYTHON_2)
    set(CMAKE_COMPILE_WARNING_AS_ERROR ON) 
endif ()

if (BUILD_SHADER_COMPILER OR BUILD_FOR_PYTHON_2)
    add_subdirectory(shadercompiler)
endif ()

add_subdirectory(trinityal)
add_subdirectory(trinity)
