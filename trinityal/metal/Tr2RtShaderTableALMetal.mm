//
//  Tr2RtShaderTableALMetal.cpp
//  ShaderCompiler
//
//  Created by Iris Dogg Skarphedinsdottir on 4.1.2024.
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_METAL

#include "Tr2RtShaderTableALMetal.h"
#include "Tr2PrimaryRenderContextMetal.h"
#include "Tr2RtPipelineStateALMetal.h"
#include "Tr2BufferALMetal.h"
#include "Tr2TextureALMetal.h"
#include "Tr2SamplerStateALMetal.h"


namespace TrinityALImpl
{
    Tr2RtShaderTableAL::Tr2RtShaderTableAL()
    {
    }

    Tr2RtShaderTableAL::~Tr2RtShaderTableAL()
    {
        Destroy();
    }

    // Shader tables contain shader records. Shader records contain a shader identifier and root arguments used to look up resources
    ALResult Tr2RtShaderTableAL::Create( const Tr2RtShaderTableDescriptionAL& desc, const ::Tr2RtPipelineStateAL& pipeline, Tr2PrimaryRenderContextAL& renderContext )
    {        
        if (@available(macOS 11.0, *)) {
            
            auto missFunctionTableDescriptor = [[MTLVisibleFunctionTableDescriptor alloc] init];
            missFunctionTableDescriptor.functionCount = desc.m_missNames.size();
            
            auto closestHitFunctionTableDescriptor = [[MTLVisibleFunctionTableDescriptor alloc] init];
            closestHitFunctionTableDescriptor.functionCount = desc.m_hitGroupNames.size();

            auto anyHitFunctionTableDescriptor = [[MTLIntersectionFunctionTableDescriptor alloc] init];
            anyHitFunctionTableDescriptor.functionCount = desc.m_hitGroupNames.size();
            // Create a table large enough to hold all of the intersection functions. Metal
            // links intersection functions into the compute pipeline state, potentially with
            // a different address for each compute pipeline. Therefore, the intersection
            // function table is specific to the compute pipeline state that created it, and you
            // can use it with only that pipeline.
            m_pipeline = pipeline.TrinityALImpl_GetObject()->GetRtPipeline();
            
            m_missShaderFunctionTable = [m_pipeline newVisibleFunctionTableWithDescriptor:missFunctionTableDescriptor];
            m_closestHitFunctionTable = [m_pipeline newVisibleFunctionTableWithDescriptor:closestHitFunctionTableDescriptor];
            m_anyHitFunctionTable = [m_pipeline newIntersectionFunctionTableWithDescriptor:anyHitFunctionTableDescriptor];
             
            // add functions to the dictionary
            auto& intersectionFunctions = pipeline.TrinityALImpl_GetObject()->GetFunctionMap();
            auto& hitGroupMap = pipeline.TrinityALImpl_GetObject()->GetHitGroupMap();
            
            const int BUFFERS_PER_SHADER = sizeof( Tr2RtLocalMaterialDescriptionAL::m_constants ) / sizeof( Tr2RtLocalMaterialDescriptionAL::m_constants[0] );
            
            auto SetFunction = [&]( auto& table, const id<MTLFunction>& function, int index )
            {
                auto handle = [m_pipeline functionHandleWithFunction:function];
                if( !handle )
                {
                    return E_INVALIDARG;
                }
                [table setFunction:handle atIndex:index];
                return S_OK;
            };
            
            auto FillMaterial = [&]( auto& ptrs, auto& material )
            {
                for( auto& cb : material.m_constants )
                {
                    uint64_t address = 0;
                    if( cb.IsValid() )
                    {
                        uint64_t offset = renderContext.UploadConstants( cb );
                        uint32_t page = uint32_t( offset >> 32 );
                        // have a vector of offsets (gpu address + addr) so for each instance it has it's own pointer into a large buffer.
                        // then bind that vector to the shadertable
                        id<MTLBuffer> cBuffer = renderContext.GetMetalContext()->GetConstantBufferAllocator().GetPage( page );
                        
                        renderContext.UseConstantBuffer(cBuffer);
                        
                        if( @available(macOS 13.0, *) )
                        {
                            address = cBuffer.gpuAddress + offset;
                        }
                    }
                    ptrs.push_back( address );
                }
            };
            
            std::vector<uint64_t> hitBuffers;
            hitBuffers.reserve( desc.m_hitGroupNames.size() * BUFFERS_PER_SHADER );
            
            // Map each piece of scene hit group to its intersection function.
            int groupIndex = 0;
            for( auto hitGroup : desc.m_hitGroupNames )
            {
                auto found = hitGroupMap.find( hitGroup.name );
                
                // if hit group is found, then get the function name (ClosestHit)
                if( found != end( hitGroupMap ) )
                {
                    if( found->second.intersection )
                    {
                        return E_FAIL;
                    }
                    if( found->second.anyHit )
                    {
                        CR_RETURN_HR( SetFunction( m_anyHitFunctionTable, found->second.anyHit, groupIndex ) );
                    }
                    else
                    {
                        [m_anyHitFunctionTable setOpaqueTriangleIntersectionFunctionWithSignature:MTLIntersectionFunctionSignatureNone atIndex:groupIndex];
                    }
                    if( found->second.closestHit )
                    {
                        CR_RETURN_HR( SetFunction( m_closestHitFunctionTable, found->second.closestHit, groupIndex ) );
                    }
                    FillMaterial( hitBuffers, hitGroup.material );
                    ++groupIndex;
                }
                else
                {
                    return E_FAIL;
                }
            }
            
            m_missMaterialOffset = hitBuffers.size() * sizeof( uint64_t );
            
            int missIndex = 0;
            for( auto& names : desc.m_missNames )
            {
                auto found = intersectionFunctions.find( names.name );
                // if we found nothing then the table is invalid
                if( found != end( intersectionFunctions ) )
                {
                    CR_RETURN_HR( !SetFunction( m_missShaderFunctionTable, found->second, missIndex ) );
                    FillMaterial( hitBuffers, names.material );
                    missIndex++;
                }
                else
                {
                    return E_FAIL;
                }
            }
            
            id<MTLDevice> device = renderContext.GetMetalContext()->GetDevice();
            if( !hitBuffers.empty() )
            {
                m_materialBuffer = [device newBufferWithBytes:hitBuffers.data()
                                                       length:hitBuffers.size() * sizeof( uint64_t )
                                                      options:MTLResourceStorageModeManaged];
                
                [m_anyHitFunctionTable setBuffer:m_materialBuffer offset:0 atIndex:0];
                
                // uncomment to debug the cbuffer
                //renderContext.UseConstantBuffer( cBuffer );
            }

            return S_OK;
        }
        else
        {
            return E_FAIL;
        }
    }

    void Tr2RtShaderTableAL::Destroy()
    {
        if( @available(macOS 11.0, *) )
        {
            m_anyHitFunctionTable = nullptr;
            m_closestHitFunctionTable = nullptr;
            m_missShaderFunctionTable = nullptr;
        }
        m_materialBuffer = nullptr;
        m_pipeline = nullptr;
    }

    bool Tr2RtShaderTableAL::IsValid() const
    {
        if( @available(macOS 11.0, *) )
        {
            return m_anyHitFunctionTable != nullptr || m_closestHitFunctionTable != nullptr || m_missShaderFunctionTable != nullptr;
        }
        return false;
    }

    id<MTLBuffer> Tr2RtShaderTableAL::GetMaterialBuffer() const
    {
        return m_materialBuffer;
    }

    NSUInteger Tr2RtShaderTableAL::GetMissMaterialOffset() const
    {
        return m_missMaterialOffset;
    }


    Tr2ALMemoryType Tr2RtShaderTableAL::GetMemoryClass() const
    {
        return AL_MEMORY_MANAGED;
    }

    void Tr2RtShaderTableAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
    {
        description["type"] = "Tr2RtShaderTableAL";
    }
}


#endif
