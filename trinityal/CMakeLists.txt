cmake_minimum_required(VERSION 3.16)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15 CACHE STRING "The minimum macOS version we target." FORCE)
endif()

project(TrinityAL VERSION 1.0.0)

set(PUBLIC_HEADER_FILES
    ALResult.h
    BcDecompress.h
    ITr2RenderContextEvents.h
    Tr2AdapterStructures.h
    Tr2DeviceResourceAL.h
    Tr2DrawUPHelper.h
    Tr2DriverUtilities.h
    Tr2HalHelperStructures.h
    Tr2LockGuard.h
    Tr2MemoryCounterAL.h
    Tr2ObjectFactory.h
    Tr2RenderContextEnum.h
    Tr2VertexDefinition.h
)

set(PUBLIC_AL_HEADER_FILES
    include/Tr2BitmapDimensions.h
    include/Tr2BufferAL.h
    include/Tr2CapsAL.h
    include/Tr2ConstantBufferAL.h
    include/Tr2CubemapFace.h
    include/Tr2FenceAL.h
    include/Tr2GpuTimerAL.h
    include/Tr2OcclusionQueryAL.h
    include/Tr2PipelineStatsQueryAL.h
    include/Tr2PixelFormat.h
    include/Tr2PrimaryRenderContextAL.h
    include/Tr2RenderContextAL.h
    include/Tr2RenderPassAL.h
    include/Tr2ResourceSetAL.h
    include/Tr2SamplerStateAL.h
    include/Tr2ShaderAL.h
    include/Tr2ShaderProgramAL.h
    include/Tr2SwapChainAL.h
    include/Tr2TextureAL.h
    include/Tr2TextureType.h
    include/Tr2VertexLayoutAL.h
    include/Tr2VideoAdapterInfoAL.h
    include/TrinityAL.h
    include/TrinityALForward.h
)

set(_SOURCES
    ALLog.h
    ALResult.cpp
    ALResult.h
    BcDecompress.cpp
    BcDecompress.h
    ITr2RenderContextEvents.h
    StdAfx.cpp
    StdAfx.h
    Tr2AdapterStructures.h
    Tr2DeviceResourceAL.cpp
    Tr2DeviceResourceAL.h
    Tr2DrawUPHelper.cpp
    Tr2DrawUPHelper.h
    Tr2DriverUtilities.cpp
    Tr2DriverUtilities.h
    Tr2HalHelperStructures.cpp
    Tr2HalHelperStructures.h
    Tr2LockGuard.cpp
    Tr2LockGuard.h
    Tr2MemoryCounterAL.cpp
    Tr2MemoryCounterAL.h
    Tr2ObjectFactory.h
    Tr2RenderContextEnum.cpp
    Tr2RenderContextEnum.h
    Tr2VertexDefinition.cpp
    Tr2VertexDefinition.h
    wglext.h
    dx11/Tr2BufferALDx11.cpp
    dx11/Tr2BufferALDx11.h
    dx11/Tr2CapsALDx11.cpp
    dx11/Tr2CapsALDx11.h
    dx11/Tr2ConstantBufferALDx11.cpp
    dx11/Tr2ConstantBufferALDx11.h
    dx11/Tr2FenceALDx11.cpp
    dx11/Tr2FenceALDx11.h
    dx11/Tr2GpuTimerALDx11.cpp
    dx11/Tr2GpuTimerALDx11.h
    dx11/Tr2OcclusionQueryALDx11.cpp
    dx11/Tr2OcclusionQueryALDx11.h
    dx11/Tr2PipelineStatsQueryALDx11.cpp
    dx11/Tr2PipelineStatsQueryALDx11.h
    dx11/Tr2PrimaryRenderContextDx11.cpp
    dx11/Tr2PrimaryRenderContextDx11.h
    dx11/Tr2RenderContextDx11.cpp
    dx11/Tr2RenderContextDx11.h
    dx11/Tr2RenderStateEmulationDx11.cpp
    dx11/Tr2RenderStateEmulationDx11.h
    dx11/Tr2ResourceSetALDx11.cpp
    dx11/Tr2ResourceSetALDx11.h
    dx11/Tr2SamplerStateALDx11.cpp
    dx11/Tr2SamplerStateALDx11.h
    dx11/Tr2ShaderALDx11.cpp
    dx11/Tr2ShaderALDx11.h
    dx11/Tr2ShaderProgramALDx11.cpp
    dx11/Tr2ShaderProgramALDx11.h
    dx11/Tr2SwapChainALDx11.cpp
    dx11/Tr2SwapChainALDx11.h
    dx11/Tr2TextureALDx11.cpp
    dx11/Tr2TextureALDx11.h
    dx11/Tr2VertexLayoutALDx11.cpp
    dx11/Tr2VertexLayoutALDx11.h
    dx11/Tr2VideoAdapterInfoALDx11.cpp
    dx11/Tr2VideoAdapterInfoALDx11.h
    dx12/Tr2BufferALDx12.cpp
    dx12/Tr2BufferALDx12.h
    dx12/Tr2CapsALDx12.h
    dx12/Tr2ConstantBufferALDx12.cpp
    dx12/Tr2ConstantBufferALDx12.h
    dx12/Tr2FenceALDx12.cpp
    dx12/Tr2FenceALDx12.h
    dx12/Tr2GpuTimerALDx12.cpp
    dx12/Tr2GpuTimerALDx12.h
    dx12/Tr2OcclusionQueryALDx12.cpp
    dx12/Tr2OcclusionQueryALDx12.h
    dx12/Tr2PipelineStatsQueryALDx12.cpp
    dx12/Tr2PipelineStatsQueryALDx12.h
    dx12/Tr2PrimaryRenderContextDx12.cpp
    dx12/Tr2PrimaryRenderContextDx12.h
    dx12/Tr2RenderContextDx12.cpp
    dx12/Tr2RenderContextDx12.h
    dx12/Tr2ResourceHelper.cpp
    dx12/Tr2ResourceHelper.h
    dx12/Tr2ResourceSetALDx12.cpp
    dx12/Tr2ResourceSetALDx12.h
    dx12/Tr2SamplerStateALDx12.cpp
    dx12/Tr2SamplerStateALDx12.h
    dx12/Tr2ShaderALDx12.cpp
    dx12/Tr2ShaderALDx12.h
    dx12/Tr2ShaderProgramALDx12.cpp
    dx12/Tr2ShaderProgramALDx12.h
    dx12/Tr2SwapChainALDx12.cpp
    dx12/Tr2SwapChainALDx12.h
    dx12/Tr2TextureALDx12.cpp
    dx12/Tr2TextureALDx12.h
    dx12/Tr2VertexLayoutALDx12.cpp
    dx12/Tr2VertexLayoutALDx12.h
    dx12/Tr2VideoAdapterInfoALDx12.cpp
    dx12/Tr2VideoAdapterInfoALDx12.h
    dx12/Utilities.cpp
    dx12/Utilities.h
    dx12/util/AmdExtDevice.h
    dx12/util/DescriptorHeapViewDx12.cpp
    dx12/util/DescriptorHeapViewDx12.h
    dx12/util/DescriptorStateCacheDx12.cpp
    dx12/util/DescriptorStateCacheDx12.h
    dx12/util/FrameLocalDescriptorHeapAllocatorDx12.cpp
    dx12/util/FrameLocalDescriptorHeapAllocatorDx12.h
    dx12/util/FrameLocalUploadBufferAllocatorDx12.cpp
    dx12/util/FrameLocalUploadBufferAllocatorDx12.h
    dx12/util/FreeList.h
    dx12/util/GlobalDescriptorHeapAllocatorDx12.cpp
    dx12/util/GlobalDescriptorHeapAllocatorDx12.h
    dx12/util/GpuCrashTracker.cpp
    dx12/util/GpuCrashTracker.h
    dx12/util/GpuMarkerBuffer.cpp
    dx12/util/GpuMarkerBuffer.h
    dx12/util/PsoDescription.cpp
    dx12/util/PsoDescription.h
    include/Tr2BitmapDimensions.h
    include/Tr2BufferAL.h
    include/Tr2CapsAL.h
    include/Tr2ConstantBufferAL.h
    include/Tr2CubemapFace.h
    include/Tr2FenceAL.h
    include/Tr2GpuTimerAL.h
    include/Tr2OcclusionQueryAL.h
    include/Tr2PipelineStatsQueryAL.h
    include/Tr2PixelFormat.h
    include/Tr2PrimaryRenderContextAL.h
    include/Tr2RenderContextAL.h
    include/Tr2RenderPassAL.h
    include/Tr2ResourceSetAL.h
    include/Tr2SamplerStateAL.h
    include/Tr2ShaderAL.h
    include/Tr2ShaderProgramAL.h
    include/Tr2SwapChainAL.h
    include/Tr2TextureAL.h
    include/Tr2TextureType.h
    include/Tr2VertexLayoutAL.h
    include/Tr2VideoAdapterInfoAL.h
    include/TrinityAL.h
    include/TrinityALForward.h
    metal/MetalContext.h
    metal/MetalContext.mm
    metal/MetalShaders.metal
    metal/MetalShaderStrings.h
    metal/MetalShaderTypes.h
    metal/MetalUtils.h
    metal/MetalUtils.mm
    metal/MetalWorkQueue.h
    metal/MetalWorkQueue.mm
    metal/Tr2BufferALMetal.h
    metal/Tr2BufferALMetal.mm
    metal/Tr2CapsALMetal.h
    metal/Tr2CapsALMetal.mm
    metal/Tr2ConstantBufferALMetal.h
    metal/Tr2ConstantBufferALMetal.mm
    metal/Tr2FenceALMetal.h
    metal/Tr2FenceALMetal.mm
    metal/Tr2GpuTimerALMetal.h
    metal/Tr2GpuTimerALMetal.mm
    metal/Tr2OcclusionQueryALMetal.h
    metal/Tr2OcclusionQueryALMetal.mm
    metal/Tr2PipelineStatsQueryALMetal.h
    metal/Tr2PipelineStatsQueryALMetal.mm
    metal/Tr2PrimaryRenderContextMetal.h
    metal/Tr2PrimaryRenderContextMetal.mm
    metal/Tr2RenderContextMetal.h
    metal/Tr2RenderContextMetal.mm
    metal/Tr2ResourceSetALMetal.h
    metal/Tr2ResourceSetALMetal.mm
    metal/Tr2SamplerStateALMetal.h
    metal/Tr2SamplerStateALMetal.mm
    metal/Tr2ShaderALMetal.h
    metal/Tr2ShaderALMetal.mm
    metal/Tr2ShaderProgramALMetal.h
    metal/Tr2ShaderProgramALMetal.mm
    metal/Tr2SwapChainALMetal.h
    metal/Tr2SwapChainALMetal.mm
    metal/Tr2TextureALMetal.h
    metal/Tr2TextureALMetal.mm
    metal/Tr2VertexLayoutALMetal.h
    metal/Tr2VertexLayoutALMetal.mm
    metal/Tr2VideoAdapterInfoALMetal.h
    metal/Tr2VideoAdapterInfoALMetal.mm
    src/Tr2BufferAL.cpp
    src/Tr2ConstantBufferAL.cpp
    src/Tr2FenceAL.cpp
    src/Tr2GpuTimerAl.cpp
    src/Tr2OcclusionQueryAL.cpp
    src/Tr2PipelineStatsQueryAL.cpp
    src/Tr2ResourceSetAL.cpp
    src/Tr2SamplerStateAL.cpp
    src/Tr2ShaderAL.cpp
    src/Tr2ShaderProgramAL.cpp
    src/Tr2SwapChainAL.cpp
    src/Tr2TextureAL.cpp
    src/Tr2VertexLayoutAL.cpp
    stub/Tr2BufferALStub.cpp
    stub/Tr2BufferALStub.h
    stub/Tr2CapsALStub.cpp
    stub/Tr2CapsALStub.h
    stub/Tr2ConstantBufferALStub.cpp
    stub/Tr2ConstantBufferALStub.h
    stub/Tr2FenceALStub.cpp
    stub/Tr2FenceALStub.h
    stub/Tr2GpuTimerALStub.cpp
    stub/Tr2GpuTimerALStub.h
    stub/Tr2OcclusionQueryALStub.cpp
    stub/Tr2OcclusionQueryALStub.h
    stub/Tr2PipelineStatsQueryALStub.cpp
    stub/Tr2PipelineStatsQueryALStub.h
    stub/Tr2PrimaryRenderContextStub.h
    stub/Tr2RenderContextStub.cpp
    stub/Tr2RenderContextStub.h
    stub/Tr2ResourceSetALStub.cpp
    stub/Tr2ResourceSetALStub.h
    stub/Tr2SamplerStateALStub.cpp
    stub/Tr2SamplerStateALStub.h
    stub/Tr2ShaderALStub.cpp
    stub/Tr2ShaderALStub.h
    stub/Tr2ShaderProgramALStub.cpp
    stub/Tr2ShaderProgramALStub.h
    stub/Tr2SwapChainALStub.cpp
    stub/Tr2SwapChainALStub.h
    stub/Tr2TextureALStub.cpp
    stub/Tr2TextureALStub.h
    stub/Tr2VertexLayoutALStub.cpp
    stub/Tr2VertexLayoutALStub.h
    stub/Tr2VideoAdapterInfoALStub.cpp
    stub/Tr2VideoAdapterInfoALStub.h
    vulkan/Tr2BufferALVulkan.cpp
    vulkan/Tr2BufferALVulkan.h
    vulkan/Tr2CapsALVulkan.h
    vulkan/Tr2ConstantBufferALVulkan.cpp
    vulkan/Tr2ConstantBufferALVulkan.h
    vulkan/Tr2FenceALVulkan.h
    vulkan/Tr2GpuTimerALVulkan.h
    vulkan/Tr2OcclusionQueryALVulkan.h
    vulkan/Tr2PipelineStatsQueryALVulkan.cpp
    vulkan/Tr2PipelineStatsQueryALVulkan.h
    vulkan/Tr2PrimaryRenderContextVulkan.cpp
    vulkan/Tr2PrimaryRenderContextVulkan.h
    vulkan/Tr2RenderContextVulkan.cpp
    vulkan/Tr2RenderContextVulkan.h
    vulkan/Tr2ResourceSetALVulkan.cpp
    vulkan/Tr2ResourceSetALVulkan.h
    vulkan/Tr2SamplerStateALVulkan.cpp
    vulkan/Tr2SamplerStateALVulkan.h
    vulkan/Tr2ShaderALVulkan.cpp
    vulkan/Tr2ShaderALVulkan.h
    vulkan/Tr2ShaderProgramALVulkan.cpp
    vulkan/Tr2ShaderProgramALVulkan.h
    vulkan/Tr2SwapChainALVulkan.h
    vulkan/Tr2TextureALVulkan.cpp
    vulkan/Tr2TextureALVulkan.h
    vulkan/Tr2VertexLayoutALVulkan.cpp
    vulkan/Tr2VertexLayoutALVulkan.h
    vulkan/Tr2VideoAdapterInfoALVulkan.cpp
    vulkan/Tr2VideoAdapterInfoALVulkan.h
    vulkan/UtilitiesVulkan.cpp
    vulkan/UtilitiesVulkan.h
    vulkan/VkResult.h
)

# Ensure that we have a sane file grouping for XCode and VS project files
if(CMAKE_GENERATOR STREQUAL "Xcode" OR CMAKE_GENERATOR MATCHES "Visual Studio.*")
    source_group(
        TREE "${CMAKE_CURRENT_SOURCE_DIR}"
        PREFIX "Sources"
        FILES ${_SOURCES}
    )
endif()

# utility function that sets properties shared between all trinityAL targets
function(set_shared_trinityal_properties targetSuffix)
    set(target TrinityAL_${targetSuffix})
    target_compile_definitions(${target}
        PUBLIC
            $<$<CONFIG:Debug>:TRINITYDEV>
        PRIVATE
            TRINITY_AL_WITH_BLUE_EXPOSURE=0
    )

    target_include_directories(${target}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/..
    )

    target_precompile_headers(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/StdAfx.h)

    target_compile_options(${target} PUBLIC "${MATH_OPTIMIZE_FLAG}")

    target_sources(${target} PRIVATE ${_SOURCES})

    find_package(CcpCore REQUIRED NO_CMAKE_PATH)

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

    add_trinity_dev_debug_flags(${target})
endfunction()

#[[
FIXME currently the _stub platform does not build on macOS due to "unknown type name: 'CGKeyCode'"
There are probably more issues because we haven't build the stub project on macOS so far IIRC
]]
if(WIN32)
    ccp_add_library(TrinityAL_stub STATIC)
    target_compile_definitions(TrinityAL_stub PUBLIC TRINITY_PLATFORM=TRINITY_STUB)
    set_shared_trinityal_properties(stub)
endif()

if(WIN32)
    # required for 11
    find_package(NvidiaAftermath REQUIRED)

    ccp_add_library(TrinityAL_dx11 STATIC)
    target_compile_definitions(TrinityAL_dx11 PUBLIC TRINITY_PLATFORM=TRINITY_DIRECTX11)
    target_link_libraries(TrinityAL_dx11 PUBLIC Nvidia::Aftermath d3d11 dxgi dxguid legacy_stdio_definitions)
    set_shared_trinityal_properties(dx11)

    ccp_add_library(TrinityAL_dx12 STATIC)
    target_compile_definitions(TrinityAL_dx12 PUBLIC TRINITY_PLATFORM=TRINITY_DIRECTX12)
    target_link_libraries(TrinityAL_dx12 PUBLIC Nvidia::Aftermath d3d12 dxgi dxguid)
    set_shared_trinityal_properties(dx12)
elseif(APPLE)
    enable_language(OBJCXX)
    ccp_add_library(TrinityAL_metal STATIC)
    target_compile_definitions(TrinityAL_metal PUBLIC TRINITY_PLATFORM=TRINITY_METAL)
    set_shared_trinityal_properties(metal)
    target_link_libraries(TrinityAL_metal PUBLIC "-framework Metal" "-framework IOKit" "-framework QuartzCore")
    target_link_libraries(TrinityAL_metal PUBLIC "-framework Metal" "-framework IOKit" "-framework QuartzCore")
    #[[
        FIXME because we're mixing objective-c++ and c++ headers in our precompiled headers then we need to force
        one specific language for macOS. This can be fixed once we revisit our usage of precompiled headers, which is
        still very MSVC specific.
    ]]
    set(_CPP_SOURCES ${_SOURCES})
    list(FILTER _CPP_SOURCES INCLUDE REGEX "\.cpp")
    set_source_files_properties(${_CPP_SOURCES} PROPERTIES LANGUAGE OBJCXX)
    target_compile_options(TrinityAL_metal PRIVATE "-fobjc-arc")
else()
    message(FATAL_ERROR "Unsupported trinity platform, only stub would be available!")
endif()


if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    option(BUILD_TESTING "Build and run tests. Enabled by default." ON)
    include(CTest)
    if(BUILD_TESTING)
	    add_subdirectory(tests)
    endif()
endif()