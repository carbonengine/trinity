//
//  Tr2RtBottomLevelAccelerationStructureMetal.m
//  carbon-trinity
//
//  Created by Iris Dogg Skarphedinsdottir on 3.1.2024.
//
#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_METAL

#include "Tr2RtBottomLevelAccelerationStructureALMetal.h"
#include "Tr2BufferALMetal.h"
#include "Tr2RenderContextMetal.h"
#include "MetalUtils.h"

namespace  TrinityALImpl {

    Tr2RtBottomLevelAccelerationStructureAL::Tr2RtBottomLevelAccelerationStructureAL()
    {
        
    }

    Tr2RtBottomLevelAccelerationStructureAL::~Tr2RtBottomLevelAccelerationStructureAL()
    {
        
    }

    // NOTE: this has no compaction
    id<MTLAccelerationStructure> Tr2RtBottomLevelAccelerationStructureAL::BuildAccelerationStructure(MTLAccelerationStructureDescriptor* descriptor, id<MTLDevice> device, MetalContext* metalContext )
    {
        // Query for the sizes needed to store and build the acceleration structure.
        MTLAccelerationStructureSizes accelSizes = [device accelerationStructureSizesWithDescriptor:descriptor];

        // Allocate an acceleration structure large enough for this descriptor. This method
        // doesn't actually build the acceleration structure, but rather allocates memory.
        id <MTLAccelerationStructure> accelerationStructure = [device newAccelerationStructureWithSize:accelSizes.accelerationStructureSize];

        // Allocate scratch space Metal uses to build the acceleration structure.
        // Use MTLResourceStorageModePrivate for the best performance because the sample
        // doesn't need access to buffer's contents.
        id <MTLBuffer> scratchBuffer = [device newBufferWithLength:accelSizes.buildScratchBufferSize options:MTLResourceStorageModeShared];

        // Create a command buffer that performs the acceleration structure build.
        id <MTLCommandBuffer> commandBuffer = [metalContext->GetCommandQueue() commandBuffer];

        // Create an acceleration structure command encoder.
        id <MTLAccelerationStructureCommandEncoder> commandEncoder = [commandBuffer accelerationStructureCommandEncoder];

        // Allocate a buffer for Metal to write the compacted accelerated structure's size into.
        //id <MTLBuffer> compactedSizeBuffer = [device newBufferWithLength:sizeof(uint32_t) options:MTLResourceStorageModeShared];

        // Schedule the actual acceleration structure build.
        [commandEncoder buildAccelerationStructure:accelerationStructure
                                        descriptor:descriptor
                                     scratchBuffer:scratchBuffer
                               scratchBufferOffset:0];
        
        // End encoding, and commit the command buffer so the GPU can start building the
        // acceleration structure.
        [commandEncoder endEncoding];

        [commandBuffer commit];
        
        m_buffer = scratchBuffer;
        
        return accelerationStructure;
    }

    // Function to build a singular BLAS, list of BLAS to be kept elsewhere
    ALResult Tr2RtBottomLevelAccelerationStructureAL::Create( const Tr2RtPositionStreamAL& positions, const Tr2RtIndicesStreamAL& indices, int numObjects, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext )
    {
        if( !renderContext.IsValid() || !renderContext.GetCaps().SupportsRaytracing() )
        {
            return E_INVALIDCALL;
        }
        if( !positions.IsValid() || !HasFlag( positions.m_vertexBuffer.GetDesc().gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
        {
            return E_INVALIDARG;
        }
        if( !indices.IsValid() || !HasFlag( indices.m_indexBuffer.GetDesc().gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
        {
            return E_INVALIDARG;
        }
        
        if (@available(macOS 11.0, *)) {

            MetalContext *metalContext = renderContext.GetMetalContext();
            
            // GEOMETRY DESCRIPTOR
            m_geomDesc = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];

            m_geomDesc.indexBuffer = indices.m_indexBuffer.TrinityALImpl_GetObject()->GetMetalBuffer();
            m_geomDesc.indexType = indices.m_stride == 2 ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32;
            m_geomDesc.indexBufferOffset = indices.m_stride * indices.m_indexOffset;
            
            m_geomDesc.vertexBuffer = positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetMetalBuffer();
            m_geomDesc.vertexStride = positions.m_stride;
            m_geomDesc.vertexBufferOffset = positions.m_vertexOffset;
             
            m_geomDesc.triangleCount = indices.m_indexCount / 3;
            
            // Acceleration structure descriptor ( a descriptor for descriptors )
            m_accelerationStructureDesc = [MTLPrimitiveAccelerationStructureDescriptor descriptor];
            m_accelerationStructureDesc.geometryDescriptors = @[m_geomDesc];
            
            id<MTLDevice> device = metalContext->GetDevice();
            
            id<MTLAccelerationStructure> primitiveAccelerationStructure = BuildAccelerationStructure(m_accelerationStructureDesc, device, metalContext);
            m_primitiveAccelerationStructure = primitiveAccelerationStructure;
            if( ( buildFlags & Tr2RtBuildFlags::ALLOW_UPDATE ) != 0 )
            {
                m_positions = positions;
                m_indices = indices;
            }
            
            return S_OK;
        }
        else
        {
            return E_INVALIDCALL;
        }
    }


    ALResult Tr2RtBottomLevelAccelerationStructureAL::Update( const Tr2RtPositionStreamAL& positions, const Tr2RtIndicesStreamAL& indices, Tr2RenderContextAL& renderContext )
    {
        if( @available(macOS 11.0, *) )
        {
            if( !renderContext.IsValid() )
            {
                return E_INVALIDARG;
            }
            if( !m_primitiveAccelerationStructure )
            {
                return E_INVALIDCALL;
            }
            
            if( m_positions.m_positionFormat        != positions.m_positionFormat                   ||
                m_positions.m_stride                != positions.m_stride                           ||
                m_positions.m_vertexCount           != positions.m_vertexCount                      ||
                m_indices.m_stride                  != indices.m_stride                             ||
                m_indices.m_indexCount              != indices.m_indexCount )
            {
                return E_INVALIDARG;
            }
            
            MetalContext *metalContext = renderContext.GetMetalContext();
            id<MTLDevice> device = metalContext->GetDevice();
            
            // Query for the sizes needed to store and build the acceleration structure.
            MTLAccelerationStructureSizes accelSizes = [device accelerationStructureSizesWithDescriptor:m_accelerationStructureDesc];
            
            id<MTLBuffer> scratchBuffer = [device newBufferWithLength:accelSizes.refitScratchBufferSize options:MTLResourceStorageModeShared];
            
            // Create a command buffer that performs the acceleration structure build.
            id <MTLCommandBuffer> commandBuffer = [metalContext->GetCommandQueue() commandBuffer];
            
            // Create an acceleration structure command encoder.
            id <MTLAccelerationStructureCommandEncoder> commandEncoder = [commandBuffer accelerationStructureCommandEncoder];
            
            // refit
            [commandEncoder refitAccelerationStructure:m_primitiveAccelerationStructure
                                            descriptor:m_accelerationStructureDesc
                                           destination:m_primitiveAccelerationStructure
                                         scratchBuffer:scratchBuffer
                                   scratchBufferOffset:0];
            
            return S_OK;
        }
        else
        {
            return E_INVALIDCALL;
        }
    }

    ALResult Tr2RtBottomLevelAccelerationStructureAL::CompactBlas( Tr2PrimaryRenderContextAL& renderContext )
    {
        return S_OK;
    }

    bool Tr2RtBottomLevelAccelerationStructureAL::IsValid() const
    {
        return m_buffer != nullptr;
    }

    Tr2ALMemoryType Tr2RtBottomLevelAccelerationStructureAL::GetMemoryClass() const
    {
        return AL_MEMORY_MANAGED;
    }
        
    void Tr2RtBottomLevelAccelerationStructureAL::Destroy()
    {
        if( @available(macOS 11.0, *) )
        {
            m_primitiveAccelerationStructure = nullptr;
            m_geomDesc = nullptr;
        }
        m_buffer = nullptr;
        m_positions = Tr2RtPositionStreamAL();
        m_indices = Tr2RtIndicesStreamAL();
    }

void Tr2RtBottomLevelAccelerationStructureAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
    {
        description["type"] = "Tr2RtBottomLevelAccelerationStructureAL";
    }

}
#endif
