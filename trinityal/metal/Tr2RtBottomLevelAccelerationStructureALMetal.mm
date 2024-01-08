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
            
             
             
             
             /*
             // AS descriptor (a descriptor for descriptors)
             let accelerationStructureDesc = MTLPrimitiveAccelerationStructureDescriptor()
             
             // Add geom descriptor to AS descriptor
             accelerationStructureDesc.geometryDescriptors = [ geometryDesc ]
             */
            
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
