project(TrinityALTest VERSION 1.0.0)

set(_SOURCES 
    ALResultTest.cpp
    BitmapDimensions.cpp
    Buffer.cpp
    Compute.cpp
    ConstantBuffer.cpp
    DepthStencil.cpp
    Dxt1Image.h
    Dxt3Image.h
    Dxt5Image.h
    Dxt5Volume.h
    FakeGoogleTest.h
    Fence.cpp
    GPUTimer.cpp
    OcclusionQuery.cpp
    RenderContext.cpp
    RenderContextCreation.cpp
    RenderTarget.cpp
    RenderWindow.h
    RenderWindow_Stub.cpp
    RenderWindow_Win32.cpp
    RenderWindow_Macos.mm
    Rendering.cpp
    SamplerState.cpp
    Shader.cpp
    StdAfx.h
    StdAfx.cpp
    SwapChain.cpp
    SwapChainResizing.cpp
    Texture.cpp
    TextureSubresource.cpp
    TrinityALTest.cpp
    VertexDefinition.cpp
    VertexLayout.cpp
    VideoAdapterInfo.cpp
    WithRenderContextFixture.h
    WithRenderContextFixture.cpp
    WithValidRenderContextFixture.h
    WithValidRenderContextFixture.cpp
    WithWindowFixture.h
    WithWindowFixture_Stub.cpp
    WithWindowFixture_Win32.cpp
    WithWindowFixture_Macos.mm
    XrgbVolume.h
)

find_package(GTest REQUIRED)

function(set_shared_trinityaltest_properties target)

    target_include_directories(${target}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/..
    )

    target_compile_definitions(${target} PRIVATE TRINITY_AL_WITH_BLUE_EXPOSURE=0 BLUEBUILD)
    target_precompile_headers(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/StdAfx.h)

    target_link_libraries(${target} PRIVATE GTest::GTest GTest::Main)

    if(APPLE)
        file(GLOB_RECURSE _CPP_SOURCES "*.cpp")
        set_source_files_properties(${_CPP_SOURCES} PROPERTIES LANGUAGE OBJCXX)
        target_compile_options(${target} PRIVATE "-fobjc-arc")
        set_target_properties(${target} PROPERTIES LINKER_LANGUAGE OBJCXX)

        target_link_libraries(${target} PRIVATE "-framework CoreFoundation")
        target_link_libraries(${target} PRIVATE "-framework Cocoa")
    elseif(WIN32)
        target_link_options(${target} PRIVATE "/IGNORE:4049")
    endif()

    # override the output directory to ensure that all TrinityALTest outputs end up in the same folder.
    # this is OK because we're differentiating them by OUTPUT_NAME
    set(output_directory "${CMAKE_SOURCE_DIR}/carbon/autobuild/TrinityALTest/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/$<$<CONFIG:DEBUG>:>")
    message(STATUS "Overriding ${target} output directory to ${output_directory}")
    set_target_properties(${target}
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${output_directory}
            PDB_OUTPUT_DIRECTORY ${output_directory}
            FOLDER Trinity/Tests
    )

endfunction()

if(WIN32)
    ccp_add_executable(TrinityALTest_dx11 ${_SOURCES})
    target_link_libraries(TrinityALTest_dx11 PRIVATE TrinityAL_dx11)
    target_compile_definitions(TrinityALTest_dx11 PRIVATE TRINITY_PLATFORM=TRINITY_DIRECTX11)
    set_shared_trinityaltest_properties(TrinityALTest_dx11)

    # This is a "temporary" workaround to allow tests as part of project build. For DX11 version
    # TrintyALTest requires Aftermath DLL. We need to copy it to the output folder so that 
    # gtest_discover_tests call would not fail.
    # If we figure out a better way of copying dlls around this will become obsolete.
    get_target_property(_AFTERMATH_DIR Nvidia::Aftermath INTERFACE_LINK_DIRECTORIES)
    get_target_property(_AFTERMATH_LIB Nvidia::Aftermath INTERFACE_LINK_LIBRARIES)
    add_custom_command(TARGET TrinityALTest_dx11 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_AFTERMATH_DIR}/${_AFTERMATH_LIB}.dll"
            $<TARGET_FILE_DIR:TrinityALTest_dx11>)

    #GTest
    set_target_properties(TrinityALTest_dx11 PROPERTIES FOLDER "TestsDX11")
    gtest_discover_tests(TrinityALTest_dx11 myListOfTests)

    ccp_add_executable(TrinityALTest_dx12 EXCLUDE_FROM_ALL ${_SOURCES})
    target_link_libraries(TrinityALTest_dx12 PRIVATE TrinityAL_dx12)
    target_compile_definitions(TrinityALTest_dx12 PRIVATE TRINITY_PLATFORM=TRINITY_DIRECTX12)
    set_shared_trinityaltest_properties(TrinityALTest_dx12)

    #GTest
    set_target_properties(TrinityALTest_dx12 PROPERTIES FOLDER "TestsDX12")
    gtest_discover_tests(TrinityALTest_dx12 myListOfTests)

elseif(APPLE)
    enable_language(OBJCXX)

    ccp_add_executable(TrinityALTest_metal ${_SOURCES})
    target_link_libraries(TrinityALTest_metal PRIVATE TrinityAL_metal)
    target_compile_definitions(TrinityALTest_metal PRIVATE TRINITY_PLATFORM=TRINITY_METAL)
    set_shared_trinityaltest_properties(TrinityALTest_metal)

    #GTest
    set_target_properties(TrinityALTest_metal PROPERTIES FOLDER "TestsMetal")
    gtest_discover_tests(TrinityALTest_metal myListOfTests)
endif()



