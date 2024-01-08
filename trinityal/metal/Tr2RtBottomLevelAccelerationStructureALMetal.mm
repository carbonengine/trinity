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
#include "MetalUtils.h"

namespace  TrinityALImpl {

    Tr2RtBottomLevelAccelerationStructureAL::Tr2RtBottomLevelAccelerationStructureAL()
    {
        
    }

    Tr2RtBottomLevelAccelerationStructureAL::~Tr2RtBottomLevelAccelerationStructureAL()
    {
        
    }

    ALResult Tr2RtBottomLevelAccelerationStructureAL::Create( const Tr2RtPositionStreamAL& positions, const Tr2RtIndicesStreamAL& indices, int numObjects, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext )
    {
        if (@available(macOS 11.0, *)) {
            //MTLResourceOptions options = getManagedBufferStorageMode();
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
            MetalContext *metalContext = renderContext.GetMetalContext();
            
            
            _primitiveAccelerationStructures = [[NSMutableArray alloc] init];
            
            // GEOMETRY DESCRIPTOR
            MTLAccelerationStructureTriangleGeometryDescriptor *geomDesc = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];

            geomDesc.indexBuffer = indices.m_indexBuffer.TrinityALImpl_GetObject()->GetMetalBuffer();
            geomDesc.indexType = indices.m_stride == 2 ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32;
            geomDesc.indexBufferOffset = indices.m_stride * indices.m_indexOffset;
            
            geomDesc.vertexBuffer = positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetMetalBuffer();
            geomDesc.vertexStride = positions.m_stride;
            //geomDesc.vertexFormat = metalContext->m_utils->GetMTLPixelFormat(positions.m_positionFormat);
            geomDesc.vertexBufferOffset = positions.m_vertexOffset;
            
            geomDesc.triangleCount = indices.m_indexBuffer.GetSize() / 3;
            
            
            // AS DESCRIPTOR ( a descriptor for descriptors)
            // Create a primitive acceleration structure descriptor to contain the single piece
            // of acceleration structure geometry.
            MTLPrimitiveAccelerationStructureDescriptor *accelerationStructureDesc = [MTLPrimitiveAccelerationStructureDescriptor descriptor];
            accelerationStructureDesc.geometryDescriptors = @[geomDesc];
            
            id<MTLDevice> device = metalContext->GetDevice();
            // Query for the sizes of the acceleration structure
            MTLAccelerationStructureSizes accelSize = [ device accelerationStructureSizesWithDescriptor:accelerationStructureDesc];
            
            // Allocate an acceleration structure large enough for this descriptor. This method
            // doesn't actually build the acceleration structure, but rather allocates memory.
            id<MTLAccelerationStructure> accelerationStructure = [device newAccelerationStructureWithSize:accelSize.accelerationStructureSize];
            
            // Allocate scratch space Metal uses to build the acceleration structure.
            // Use MTLResourceStorageModePrivate for the best performance because the sample
            // doesn't need access to buffer's contents.
            id<MTLBuffer> scratchBuffer = [device newBufferWithLength:accelSize.buildScratchBufferSize options:MTLResourceStorageModePrivate];
            
            // Create a command buffer that performs the acceleration structure build
            id<MTLCommandBuffer> commandBuffer = [metalContext->GetCommandQueue() commandBuffer];
            
            // Create an acceleration structure command encoder
            id<MTLAccelerationStructureCommandEncoder> commandEncoder = [commandBuffer accelerationStructureCommandEncoder];
            
            // Allocate a buffer for metal to write the compacted accelerated structure's size into.
            id<MTLBuffer> sizeBuffer = [device newBufferWithLength:sizeof(uint32_t) options:MTLResourceStorageModeShared];
            
            // Schedule the actual acceleration structure build
            [commandEncoder buildAccelerationStructure:accelerationStructure
                                            descriptor:accelerationStructureDesc
                                         scratchBuffer:scratchBuffer
                                   scratchBufferOffset:0];
            
            
            // Compute and write the scratch size acceleration structure size into the buffer
            //[commandEncoder writeCompactedAccelerationStructureSize:accelerationStructure
            //                                               toBuffer:sizeBuffer
            //                                               offset:0];
            
            // End encoding, and commit the command buffer so the GPU can start building the
            // acceleration structure.
            [commandEncoder endEncoding];

            [commandBuffer commit];
            
            // NOTE: If we want to compact the AS we should do it here, that is call commandBuffer waitUntilCompleted
            // and then read the compactedSize then allocate that size to the AS.
            // but we need CPU/GPU synchronization for compaction, also we don't want to compact AS that we rebuild every frame

                    
            
            // AFTER BUILDING Add the acceleration structure to the array of primitive acceleration structures.
            [_primitiveAccelerationStructures addObject:accelerationStructure];
            return S_OK;
        }
        else
        {
            return S_OK;
        }
    }


    ALResult Tr2RtBottomLevelAccelerationStructureAL::Update( const Tr2RtPositionStreamAL& positions, const Tr2RtIndicesStreamAL& indices, Tr2RenderContextAL& renderContext )
    {
        return S_OK;
    }

    ALResult Tr2RtBottomLevelAccelerationStructureAL::CompactBlas( Tr2PrimaryRenderContextAL& renderContext )
    {
        return S_OK;
    }

    bool Tr2RtBottomLevelAccelerationStructureAL::IsValid() const
    {
        return true;
       // if( m_primitiveAccelerationStructures )
        //{
       //     return true;
       // }
    }

    Tr2ALMemoryType Tr2RtBottomLevelAccelerationStructureAL::GetMemoryClass() const
    {
        return AL_MEMORY_MANAGED;
    }
        
    void Tr2RtBottomLevelAccelerationStructureAL::Destroy()
    {
        
    }

void Tr2RtBottomLevelAccelerationStructureAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
    {
        description["type"] = "Tr2RtBottomLevelAccelerationStructureAL";
    }

}
#endif
