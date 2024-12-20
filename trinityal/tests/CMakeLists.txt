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
    Raytracing.cpp
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

set(_SHADERS_METAL
    Shaders.metal/AddConstantToBuffer.csh
    Shaders.metal/AddVectors.csh
    Shaders.metal/ClosestHit.rsh
    Shaders.metal/ClosestHitWithMaterial.rsh
    Shaders.metal/ConstantColor.psh
    Shaders.metal/GroupShared.csh
    Shaders.metal/InstancedRendering.vsh
    Shaders.metal/LoadMsaaTexture.psh
    Shaders.metal/MetalDefines.h
    Shaders.metal/Miss.rsh
    Shaders.metal/OutputTexCoord.psh
    Shaders.metal/OutputVector.csh
    Shaders.metal/PositionOnly.vsh
    Shaders.metal/PositionOnlyWithPerObjectData.vsh
    Shaders.metal/RayGen.rsh
    Shaders.metal/SampleTexture.csh
    Shaders.metal/SampleTextureFromTexCoord.psh
    Shaders.metal/SampleTextureMipFromTexCoord.psh
    Shaders.metal/SampleVolumeTexture.psh
    Shaders.metal/TexCoordAndPosition.vsh
    Shaders.metal/WriteToUav.psh
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

    #GTest
    set_target_properties(TrinityALTest_dx11 PROPERTIES FOLDER "Tests")
    #gtest_discover_tests(TrinityALTest_dx11 myListOfTests)

    # Copy DLL dependencies
    add_custom_command(
        TARGET TrinityALTest_dx11 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:TrinityALTest_dx11> $<TARGET_FILE_DIR:TrinityALTest_dx11>
        COMMAND_EXPAND_LISTS
    )

    ccp_add_executable(TrinityALTest_dx12 ${_SOURCES})
    target_link_libraries(TrinityALTest_dx12 PRIVATE TrinityAL_dx12)
    target_compile_definitions(TrinityALTest_dx12 PRIVATE TRINITY_PLATFORM=TRINITY_DIRECTX12)
    set_shared_trinityaltest_properties(TrinityALTest_dx12)

    # Copy DLL dependencies
    add_custom_command(
        TARGET TrinityALTest_dx12 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:TrinityALTest_dx12> $<TARGET_FILE_DIR:TrinityALTest_dx12>
        COMMAND_EXPAND_LISTS
    )

    #GTest
    set_target_properties(TrinityALTest_dx12 PROPERTIES FOLDER "Tests")
    #gtest_discover_tests(TrinityALTest_dx12 myListOfTests)

elseif(APPLE)
    enable_language(OBJCXX)

    ccp_add_executable(TrinityALTest_metal ${_SOURCES})
    target_link_libraries(TrinityALTest_metal PRIVATE TrinityAL_metal)
    target_compile_definitions(TrinityALTest_metal PRIVATE TRINITY_PLATFORM=TRINITY_METAL)
    set_shared_trinityaltest_properties(TrinityALTest_metal)

    #GTest
    set_target_properties(TrinityALTest_metal PROPERTIES FOLDER "Tests")
    #gtest_discover_tests(TrinityALTest_metal myListOfTests DISCOVERY_MODE PRE_TEST)

    set(GENERATED_SOURCES_DIR ${CMAKE_CURRENT_BINARY_DIR}/GeneratedSources)

    target_sources(TrinityALTest_metal PRIVATE ${_SHADERS_METAL})
    target_include_directories(TrinityALTest_metal PRIVATE ${GENERATED_SOURCES_DIR})
    source_group(Shaders FILES ${_SHADERS_METAL})

    foreach(shaderSource ${_SHADERS_METAL})
        string(REGEX REPLACE "h$" ".h" shaderOutput ${shaderSource})
        add_custom_command(
            OUTPUT ${GENERATED_SOURCES_DIR}/${shaderOutput}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_SOURCES_DIR}/Shaders.metal
            COMMAND xcrun metal -x metal -std=metal3.0 -gline-tables-only -MO ${CMAKE_CURRENT_SOURCE_DIR}/${shaderSource} -o ${GENERATED_SOURCES_DIR}/${shaderSource}.bin
            COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/bin2h.cmd ${GENERATED_SOURCES_DIR}/${shaderSource}.bin ${GENERATED_SOURCES_DIR}/${shaderOutput}
            MAIN_DEPENDENCY ${shaderSource}
        )
    endforeach()

    # Copy DLL dependencies; there must be a better way :(
    add_custom_command(
        TARGET TrinityALTest_metal POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CcpLog_DIR}/${CCP_VENDOR_LIB_PATH}/libCcpLog.dylib $<TARGET_FILE_DIR:TrinityALTest_metal>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${Tracy_DIR}/${CCP_VENDOR_LIB_PATH}/libTracyClient.0.11.1.dylib $<TARGET_FILE_DIR:TrinityALTest_metal>
    )

endif()



