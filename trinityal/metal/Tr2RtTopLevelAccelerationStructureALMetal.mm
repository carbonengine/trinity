//
//  Tr2RtTopLevelAccelerationStructureALMetal.cpp
//  carbon-trinity
//
//  Created by Iris Dogg Skarphedinsdottir on 3.1.2024.
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_METAL

#include "Tr2RtTopLevelAccelerationStructureALMetal.h"
#include "Tr2BufferALMetal.h"

namespace TrinityALImpl
{

    

Tr2RtTopLevelAccelerationStructureAL::Tr2RtTopLevelAccelerationStructureAL()
    {
    }


    Tr2RtTopLevelAccelerationStructureAL::~Tr2RtTopLevelAccelerationStructureAL()
    {
    }

    // NOTE: this has no compaction
    id<MTLAccelerationStructure> Tr2RtTopLevelAccelerationStructureAL::BuildAccelerationStructure(MTLAccelerationStructureDescriptor* descriptor, id<MTLDevice> device, MetalContext* metalContext)
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
        id <MTLBuffer> compactedSizeBuffer = [device newBufferWithLength:sizeof(uint32_t) options:MTLResourceStorageModeShared];

        // Schedule the actual acceleration structure build.
        [commandEncoder buildAccelerationStructure:accelerationStructure
                                        descriptor:descriptor
                                     scratchBuffer:scratchBuffer
                               scratchBufferOffset:0];
        
        // Compute and write the compacted acceleration structure size into the buffer. You
        // must already have a built acceleration structure because Metal determines the compacted
        // size based on the final size of the acceleration structure. Compacting an acceleration
        // structure can potentially reclaim significant amounts of memory because Metal must
        // create the initial structure using a conservative approach.

        [commandEncoder writeCompactedAccelerationStructureSize:accelerationStructure
                                                       toBuffer:compactedSizeBuffer
                                                         offset:0];
        
        // End encoding, and commit the command buffer so the GPU can start building the
        // acceleration structure.
        [commandEncoder endEncoding];

        [commandBuffer commit];
        
        // Note: Don't wait for Metal to finish executing the command buffer if you aren't compacting
        // the acceleration structure, as doing so requires CPU/GPU synchronization. You don't have
        // to compact acceleration structures, but do so when creating large static acceleration
        // structures, such as static scene geometry. Avoid compacting acceleration structures that
        // you rebuild every frame, as the synchronization cost may be significant.

        [commandBuffer waitUntilCompleted];
        
        uint32_t compactedSize = *(uint32_t *)compactedSizeBuffer.contents;
        
        // Allocate a smaller acceleration structure based on the returned size.
        id <MTLAccelerationStructure> compactedAccelerationStructure = [device newAccelerationStructureWithSize:compactedSize];
        
        
        // Create a new command buffer that performs the acceleration structure build.
        commandBuffer = [metalContext->GetCommandQueue() commandBuffer];
        commandEncoder = [commandBuffer accelerationStructureCommandEncoder];
        
        // Encode the command to copy and compact the acceleration structure into the
        // smaller acceleration structure.
        [commandEncoder copyAndCompactAccelerationStructure:accelerationStructure
                                    toAccelerationStructure:compactedAccelerationStructure];
        
        // End encoding and commit the command buffer. You don't need to wait for Metal to finish
        // executing this command buffer as long as you synchronize any ray-intersection work
        // to run after this command buffer completes. The sample relies on Metal's default
        // dependency tracking on resources to automatically synchronize access to the new
        // compacted acceleration structure.
        [commandEncoder endEncoding];
        [commandBuffer commit];

        return compactedAccelerationStructure;
    }

    ALResult Tr2RtTopLevelAccelerationStructureAL::Create( size_t count, const Tr2RtInstanceAL* instances, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext )
    {
        if (@available(macOS 11.0, *)) {
            if( !renderContext.IsValid() || !renderContext.GetCaps().SupportsRaytracing() )
            {
                return E_INVALIDCALL;
            }
            if( count == 0 )
            {
                return E_INVALIDARG;
            }
            
            m_primitiveAccelerationStructures = [[NSMutableArray alloc] init];
            
            MetalContext *metalContext = renderContext.GetMetalContext();
            id<MTLDevice> device = metalContext->GetDevice();
      
            // Allocate a buffer of acceleration structure instance descriptors. Each descriptor represents
            // an instance of one of the primitive acceleration structures created above, with its own
            // transformation matrix.
            
            // maybe the option should be MTLResourceStorageModePrivate
            m_instanceBuffer = [device newBufferWithLength:sizeof(MTLAccelerationStructureInstanceDescriptor) * count options:MTLResourceStorageModeShared];
            
            MTLAccelerationStructureInstanceDescriptor *instanceDescriptors = (MTLAccelerationStructureInstanceDescriptor *)m_instanceBuffer.contents;

            // Fill out instance descriptors.
            for (NSUInteger instanceIndex = 0; instanceIndex < count; instanceIndex++) {
                
                if( !instances[instanceIndex].blas.IsValid())
                {
                    return E_INVALIDARG;
                }
                
                // TODO: FIX THIS
                // it's supposed to map the instance to its acceleration structure.
                instanceDescriptors[instanceIndex].accelerationStructureIndex = (uint32_t)instanceIndex;

                // Mark the instance as opaque so that the
                // ray intersector doesn't attempt to execute a function that doesn't exist.
                instanceDescriptors[instanceIndex].options = MTLAccelerationStructureInstanceOptionOpaque;

                
                // Metal adds the geometry intersection function table offset and instance intersection
                // function table offset together to determine which intersection function to execute.
                // The sample mapped geometries directly to their intersection functions above, so it
                // sets the instance's table offset to 0.
                instanceDescriptors[instanceIndex].intersectionFunctionTableOffset = 0;
                
                // Set the instance mask, which the sample uses to filter out intersections between rays
                // and geometry. For example, it uses masks to prevent light sources from being visible
                // to secondary rays, which would result in their contribution being double-counted.
                instanceDescriptors[instanceIndex].mask = 3;
                
                // Copy the first three rows of the instance transformation matrix. Metal
                // assumes that the bottom row is (0, 0, 0, 1), which allows the renderer to
                // tightly pack instance descriptors in memory.
                for(int column = 0; column < 4; column++)
                    for(int row = 0; row < 3; row++)
                        instanceDescriptors[instanceIndex].transformationMatrix.columns[column][row] = instances[instanceIndex].transform[row][column];
                
                id<MTLAccelerationStructure> blas = instances[instanceIndex].blas.TrinityALImpl_GetObject()->m_primitiveAccelerationStructure;
                //gather all the BLAS in one list
                [m_primitiveAccelerationStructures addObject:blas];
            }
                
            // Create an instance acceleration structure descriptor
            MTLInstanceAccelerationStructureDescriptor *accelDescriptor = [MTLInstanceAccelerationStructureDescriptor descriptor];
            
            accelDescriptor.instancedAccelerationStructures = m_primitiveAccelerationStructures;
            accelDescriptor.instanceCount = count;
            accelDescriptor.instanceDescriptorBuffer = m_instanceBuffer;
            
            // Create the instance acceleration structure that contains all instances in the scene.
            m_instanceAccelerationStructure = BuildAccelerationStructure( accelDescriptor, device, metalContext );
            
            // add vector of contents
            // gpu usage is probs srv and cpu none
            if (@available(macOS 13.0, *)) {
                Tr2BufferDescriptionAL bufferDesc = Tr2BufferDescriptionAL(1, sizeof(m_instanceAccelerationStructure.gpuResourceID), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE);
                
                auto gpuID = m_instanceAccelerationStructure.gpuResourceID;
                
                m_buffer.Create(bufferDesc, &gpuID, renderContext);
                m_buffer.SetName("TLASBuffer");
            }
            
            id <MTLComputeCommandEncoder> computeEncoder = metalContext->GetPrimaryWorkQueue()->GetComputeEncoder();
            [computeEncoder setAccelerationStructure:m_instanceAccelerationStructure atBufferIndex:5];
            
            metalContext->GetPrimaryWorkQueue()->ReleaseEncoder( false );
            
            return S_OK;
        }
        else
        {
            return E_FAIL;
        }
        
    }

    ALResult Tr2RtTopLevelAccelerationStructureAL::Update( size_t count, const Tr2RtInstanceAL* instances, Tr2RenderContextAL& renderContext )
    {
        return E_FAIL;
    }

    bool Tr2RtTopLevelAccelerationStructureAL::IsValid() const
    {
        return m_instanceBuffer != nullptr;
    }
    
    const ::Tr2BufferAL& Tr2RtTopLevelAccelerationStructureAL::GetBuffer() const
    {
        return m_buffer;
        //return nullptr;
    }

    size_t Tr2RtTopLevelAccelerationStructureAL::GetCapacity() const
    {
        return 0;
    }

    Tr2ALMemoryType Tr2RtTopLevelAccelerationStructureAL::GetMemoryClass() const
    {
        return AL_MEMORY_MANAGED;
    }

    void Tr2RtTopLevelAccelerationStructureAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
    {
        description["type"] = "Tr2RtTopLevelAccelerationStructureAL";
    }

    void Tr2RtTopLevelAccelerationStructureAL::Destroy()
    {
    
    }
}

#endif
