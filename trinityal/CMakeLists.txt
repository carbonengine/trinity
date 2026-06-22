# Copyright © 2023 CCP ehf.

cmake_minimum_required(VERSION 3.16)

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
    include/Tr2BufferAL.h
    include/Tr2CapsAL.h
    include/Tr2ConstantBufferAL.h
    include/Tr2FenceAL.h
    include/Tr2GpuTimerAL.h
    include/Tr2OcclusionQueryAL.h
    include/Tr2PipelineStatsQueryAL.h
    include/Tr2PrimaryRenderContextAL.h
    include/Tr2RenderContextAL.h
    include/Tr2RenderPassAL.h
    include/Tr2ResourceSetAL.h
    include/Tr2SamplerStateAL.h
    include/Tr2ShaderAL.h
    include/Tr2ShaderProgramAL.h
    include/Tr2SwapChainAL.h
    include/Tr2TextureAL.h
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
    dx11/upscaling/Tr2DlssUpscaling.cpp 
    dx11/upscaling/Tr2DlssUpscaling.h
    dx11/upscaling/Tr2UpscalingALDx11.cpp
    dx11/upscaling/Tr2UpscalingALDx11.h
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
    dx12/Tr2RtBottomLevelAccelerationStructureALDx12.cpp
    dx12/Tr2RtBottomLevelAccelerationStructureALDx12.h
    dx12/Tr2RtPipelineStateALDx12.cpp
    dx12/Tr2RtPipelineStateALDx12.h
    dx12/Tr2RtShaderTableALDx12.cpp
    dx12/Tr2RtShaderTableALDx12.h
    dx12/Tr2RtTopLevelAccelerationStructureALDx12.cpp
    dx12/Tr2RtTopLevelAccelerationStructureALDx12.h
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
    dx12/upscaling/Tr2DlssUpscaling.cpp 
    dx12/upscaling/Tr2DlssUpscaling.h 
    dx12/upscaling/Tr2Fsr3Upscaling.cpp 
    dx12/upscaling/Tr2Fsr3Upscaling.h 
    dx12/upscaling/Tr2UpscalingALDx12.cpp
    dx12/upscaling/Tr2UpscalingALDx12.h
    dx12/upscaling/Tr2XessUpscaling.cpp   
    dx12/upscaling/Tr2XessUpscaling.h  
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
    include/Tr2BufferAL.h
    include/Tr2CapsAL.h
    include/Tr2ConstantBufferAL.h
    include/Tr2FenceAL.h
    include/Tr2GpuTimerAL.h
    include/Tr2OcclusionQueryAL.h
    include/Tr2RtBottomLevelAccelerationStructureAL.h
    include/Tr2RtPipelineStateAL.h
    include/Tr2RtShaderTableAL.h
    include/Tr2RtTopLevelAccelerationStructureAL.h
    include/Tr2PipelineStatsQueryAL.h
    include/Tr2PrimaryRenderContextAL.h
    include/Tr2RenderContextAL.h
    include/Tr2RenderPassAL.h
    include/Tr2ResourceSetAL.h
    include/Tr2SamplerStateAL.h
    include/Tr2ShaderAL.h
    include/Tr2ShaderProgramAL.h    
    include/Tr2StreamlineAL.h    
    include/Tr2SwapChainAL.h
    include/Tr2TextureAL.h
    include/Tr2VertexLayoutAL.h
    include/Tr2VideoAdapterInfoAL.h
    include/TrinityAL.h
    include/TrinityALForward.h
    include/upscaling/Tr2Fsr1Upscaling.h
    include/upscaling/Tr2UpscalingAL.h
    include/upscaling/Fsr1DX.h
    include/upscaling/Fsr1Metal.h
    metal/MetalContext.h
    metal/MetalContext.mm
    metal/MetalResourceArray.h
    metal/MetalResourceArray.mm
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
    metal/Tr2RtBottomLevelAccelerationStructureALMetal.mm
    metal/Tr2RtBottomLevelAccelerationStructureALMetal.h
    metal/Tr2RtPipelineStateALMetal.mm
    metal/Tr2RtPipelineStateALMetal.h
    metal/Tr2RtShaderTableALMetal.mm
    metal/Tr2RtShaderTableALMetal.h
    metal/Tr2RtTopLevelAccelerationStructureALMetal.mm
    metal/Tr2RtTopLevelAccelerationStructureALMetal.h
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
    metal/upscaling/Tr2MetalFxUpscaling.h 
    metal/upscaling/Tr2MetalFxUpscaling.mm
    metal/upscaling/Tr2UpscalingALMetal.h
    metal/upscaling/Tr2UpscalingALMetal.mm
    src/Tr2BufferAL.cpp
    src/Tr2ConstantBufferAL.cpp
    src/Tr2FenceAL.cpp
    src/Tr2GpuTimerAl.cpp
    src/Tr2OcclusionQueryAL.cpp
    src/Tr2PipelineStatsQueryAL.cpp
    src/Tr2ResourceSetAL.cpp
    src/Tr2RtBottomLevelAccelerationStructureAL.cpp
    src/Tr2RtPipelineStateAL.cpp
    src/Tr2RtShaderTableAL.cpp
    src/Tr2RtTopLevelAccelerationStructureAL.cpp
    src/Tr2SamplerStateAL.cpp
    src/Tr2ShaderAL.cpp
    src/Tr2ShaderProgramAL.cpp    
    src/Tr2StreamlineAL.cpp    
    src/Tr2SwapChainAL.cpp
    src/Tr2TextureAL.cpp
    src/Tr2VertexLayoutAL.cpp
    src/upscaling/Tr2Fsr1Upscaling.cpp
    src/upscaling/Tr2UpscalingAL.cpp
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
    stub/upscaling/Tr2UpscalingALStub.cpp
    stub/upscaling/Tr2UpscalingALStub.h
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

    if(BUILD_FOR_PYTHON_2)
        find_package(CcpCore REQUIRED NO_CMAKE_PATH)
    else()
        find_package(carbon-core CONFIG REQUIRED)
    endif()

    target_link_libraries(${target} PUBLIC CcpCore CcpImageIO)

    # override the output directory to ensure that all TrinityAL outputs end up in the same folder.
    # this is OK because we're differentiating them by OUTPUT_NAME
    set(output_directory "${CMAKE_BINARY_DIR}/carbon/autobuild/TrinityAL/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/$<$<CONFIG:DEBUG>:>")
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
    
    ccp_enable_vs_analizers(${target})
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

if(BUILD_FOR_PYTHON_2)
    find_package(ImageIO REQUIRED NO_CMAKE_PATH)
    add_library(CcpImageIO ALIAS ImageIO)
else()
    find_package(carbon-imageio CONFIG REQUIRED)
endif()

if(WIN32)

    if (BUILD_DX11 OR BUILD_DX12)
        if (BUILD_FOR_PYTHON_2)
            find_package(NvidiaAftermath REQUIRED)
            add_library(NVIDIA::Aftermath ALIAS Nvidia::Aftermath)
            find_package(NvidiaStreamline REQUIRED)
            find_package(NvidiaDlssPlugin REQUIRED)
            add_library(NVIDIA::Streamline::dlss-sr ALIAS NVIDIA::dlss_sl_plugin )
            add_library(NVIDIA::Streamline::dlss-fg INTERFACE IMPORTED)
            find_package(NvidiaNisPlugin REQUIRED)
            add_library(NVIDIA::Streamline::nis ALIAS NVIDIA::nis_sl_plugin)
            find_package(NvidiaPclPlugin REQUIRED)
            find_package(FidelityFX_FSR REQUIRED)
            add_library(AMD::FidelityFX::fsr ALIAS AMD::FidelityFX_FSR)
        else()
            find_package(nvidia-aftermath CONFIG REQUIRED)
            find_package(nvidia-streamline CONFIG REQUIRED)
            find_package(nvidia-streamline-dlss-sr CONFIG REQUIRED)
            find_package(nvidia-streamline-dlss-fg CONFIG REQUIRED)
            find_package(amd-fidelityfx-fsr REQUIRED)
            find_package(nvidia-streamline-nis CONFIG REQUIRED)
        endif()
        
    endif()

    if (BUILD_DX11)
    
        ccp_add_library(TrinityAL_dx11 STATIC)
        target_compile_definitions(TrinityAL_dx11 PUBLIC TRINITY_PLATFORM=TRINITY_DIRECTX11)
        target_link_libraries(TrinityAL_dx11 PUBLIC d3d11 
                                                    dxgi
                                                    dxguid
                                                    NVIDIA::Aftermath  
                                                    NVIDIA::Streamline 
                                                    NVIDIA::Streamline::dlss-sr
                                                    NVIDIA::Streamline::dlss-fg
                                                    NVIDIA::Streamline::nis 
                                                    AMD::FidelityFX::fsr
                                                    legacy_stdio_definitions)
        set_shared_trinityal_properties(dx11)
    endif()

    if (BUILD_DX12)

        if (BUILD_FOR_PYTHON_2)
            find_package(FidelityFX REQUIRED)
            find_package(IntelXeSS REQUIRED)
            add_library(INTEL::Xess ALIAS INTEL::xess)
            find_package(NvidiaReflexPlugin REQUIRED)
            add_library(NVIDIA::Streamline::reflex ALIAS NVIDIA::reflex_sl_plugin )
        else()
            find_package(amd-fidelityfx REQUIRED)
            find_package(intel-xess REQUIRED)
            find_package(nvidia-streamline-reflex CONFIG REQUIRED)
        endif()

        ccp_add_library(TrinityAL_dx12 STATIC)
        target_compile_definitions(TrinityAL_dx12 PUBLIC TRINITY_PLATFORM=TRINITY_DIRECTX12)
        target_link_libraries(TrinityAL_dx12 PUBLIC d3d12 
                                                    dxgi 
                                                    dxguid 
                                                    NVIDIA::Aftermath  
                                                    NVIDIA::Streamline 
                                                    NVIDIA::Streamline::dlss-sr
                                                    NVIDIA::Streamline::dlss-fg
                                                    NVIDIA::Streamline::nis 
                                                    NVIDIA::Streamline::reflex 
                                                    AMD::FidelityFX 
                                                    AMD::FidelityFX::fsr
                                                    INTEL::Xess)

        if (BUILD_FOR_PYTHON_2)
            target_link_libraries(TrinityAL_dx12 PUBLIC NVIDIA::pcl_sl_plugin)
        endif()

        set_shared_trinityal_properties(dx12)
    endif()

elseif(APPLE)
    
    if(BUILD_METAL)

        if (BUILD_FOR_PYTHON_2)
            find_package(FidelityFX_FSR REQUIRED)
            add_library(AMD::FidelityFX::fsr ALIAS AMD::FidelityFX_FSR)
        else()
            find_package(amd-fidelityfx-fsr REQUIRED)
        endif()
        
        enable_language(OBJCXX)
        ccp_add_library(TrinityAL_metal STATIC)
        target_compile_definitions(TrinityAL_metal PUBLIC TRINITY_PLATFORM=TRINITY_METAL)
        set_shared_trinityal_properties(metal)
        target_link_libraries(TrinityAL_metal PUBLIC "-framework Metal" "-framework MetalFX" "-framework IOKit" "-framework QuartzCore" AMD::FidelityFX::fsr)
        #[[
            FIXME because we're mixing objective-c++ and c++ headers in our precompiled headers then we need to force
            one specific language for macOS. This can be fixed once we revisit our usage of precompiled headers, which is
            still very MSVC specific.
        ]]
        set(_CPP_SOURCES ${_SOURCES})
        list(FILTER _CPP_SOURCES INCLUDE REGEX "\.cpp")
        set_source_files_properties(${_CPP_SOURCES} PROPERTIES LANGUAGE OBJCXX)
        target_compile_options(TrinityAL_metal PRIVATE "-fobjc-arc")
    endif()
else()
    message(FATAL_ERROR "Unsupported trinity platform, only stub would be available!")
endif()

option(BUILD_TESTING "Build and run tests. Enabled by default." ON)
message(STATUS " BUILD_TESTING VALUE IS ${BUILD_TESTING}")
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
