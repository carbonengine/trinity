project(TrinityAL VERSION 1.0.0)

# utility function that sets properties shared between all trinityAL targets
function(set_shared_trinityal_properties target)
    target_compile_definitions(${target}
        PUBLIC
            $<$<CONFIG:Debug>:TRINITYDEV>
        PRIVATE
            TRINITY_AL_WITH_BLUE_EXPOSURE=0
    )

    target_include_directories(${target}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/..
    )

    target_precompile_headers(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/StdAfx.h)

    # FIXME we should really not be globbing here
    file(GLOB_RECURSE _SOURCES "*.cpp" "*.mm" "*.h")
    target_sources(${target} PRIVATE ${_SOURCES})
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
            PREFIX "Sources"
            FILES ${_SOURCES}
    )

    target_link_libraries(${target} PUBLIC CcpCore)

    # override the output directory to ensure that all TrinityAL outputs end up in the same folder.
    # this is OK because we're differentiating them by OUTPUT_NAME
    set(output_directory "${CMAKE_SOURCE_DIR}/carbon/autobuild/TrinityAL/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/$<$<CONFIG:DEBUG>:>")
    message(STATUS "Overriding ${target} output directory to ${output_directory}")
    set_target_properties(${target}
        PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${output_directory}
            LIBRARY_OUTPUT_DIRECTORY ${output_directory}
            RUNTIME_OUTPUT_DIRECTORY ${output_directory}
            PDB_OUTPUT_DIRECTORY ${output_directory}
            FOLDER Trinity
            POSITION_INDEPENDENT_CODE ON
            EXPORT_NAME TrinityAL
    )
endfunction()

#[[
FIXME currently the _stub platform does not build on macOS due to "unknown type name: 'CGKeyCode'"
There are probably more issues because we haven't build the stub project on macOS so far IIRC
]]
if(WIN32)
add_library(TrinityAL_stub STATIC EXCLUDE_FROM_ALL)
target_compile_definitions(TrinityAL_stub PUBLIC TRINITY_PLATFORM=TRINITY_STUB)
set_shared_trinityal_properties(TrinityAL_stub)
endif()

if(WIN32)
    # required for dx9 and 11
    find_package(MicrosoftDirectX9 REQUIRED)
    find_package(NvidiaAftermath REQUIRED)

    add_library(TrinityAL_dx9 STATIC EXCLUDE_FROM_ALL)
    target_compile_definitions(TrinityAL_dx9 PUBLIC TRINITY_PLATFORM=TRINITY_DIRECTX9)
    target_link_libraries(TrinityAL_dx9 PUBLIC Microsoft::DirectX9 Nvidia::Aftermath legacy_stdio_definitions)
    set_shared_trinityal_properties(TrinityAL_dx9)

    add_library(TrinityAL_dx11 STATIC)
    target_compile_definitions(TrinityAL_dx11 PUBLIC TRINITY_PLATFORM=TRINITY_DIRECTX11)
    target_link_libraries(TrinityAL_dx11 PUBLIC Microsoft::DirectX9 Nvidia::Aftermath d3d11 dxgi legacy_stdio_definitions)
    set_shared_trinityal_properties(TrinityAL_dx11)

    add_library(TrinityAL_dx12 STATIC EXCLUDE_FROM_ALL)
    target_compile_definitions(TrinityAL_dx12 PUBLIC TRINITY_PLATFORM=TRINITY_DIRECTX12)
    target_link_libraries(TrinityAL_dx12 PUBLIC d3d12 dxgi)
    set_shared_trinityal_properties(TrinityAL_dx12)
elseif(APPLE)
    enable_language(OBJCXX)
    add_library(TrinityAL_metal STATIC)
    target_compile_definitions(TrinityAL_metal PUBLIC TRINITY_PLATFORM=TRINITY_METAL)
    set_shared_trinityal_properties(TrinityAL_metal)
    target_link_libraries(TrinityAL_metal PUBLIC "-framework Metal" "-framework IOKit" "-framework QuartzCore")
    target_link_libraries(TrinityAL_metal PUBLIC "-framework Metal" "-framework IOKit" "-framework QuartzCore")
    #[[
        FIXME because we're mixing objective-c++ and c++ headers in our precompiled headers then we need to force
        one specific language for macOS. This can be fixed once we revisit our usage of precompiled headers, which is
        still very MSVC specific.
    ]]
    file(GLOB_RECURSE _CPP_SOURCES "*.cpp")
    set_source_files_properties(${_CPP_SOURCES} PROPERTIES LANGUAGE OBJCXX)
    target_compile_options(TrinityAL_metal PRIVATE "-fobjc-arc")
else()
    message(FATAL_ERROR "Unsupported trinity platform, only stub would be available!")
endif()
