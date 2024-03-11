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
            
            MTLVisibleFunctionTableDescriptor *missFunctionTableDescriptor = [[MTLVisibleFunctionTableDescriptor alloc] init];
            missFunctionTableDescriptor.functionCount = desc.m_missNames.size();
            
            MTLIntersectionFunctionTableDescriptor *hitGroupFunctionTableDescriptor = [[MTLIntersectionFunctionTableDescriptor alloc] init];
            hitGroupFunctionTableDescriptor.functionCount = desc.m_hitGroupNames.size();
            // Create a table large enough to hold all of the intersection functions. Metal
            // links intersection functions into the compute pipeline state, potentially with
            // a different address for each compute pipeline. Therefore, the intersection
            // function table is specific to the compute pipeline state that created it, and you
            // can use it with only that pipeline.
            m_pipeline = pipeline.TrinityALImpl_GetObject()->GetRtPipeline();
            
            m_missShaderFunctionTable = [m_pipeline newVisibleFunctionTableWithDescriptor:missFunctionTableDescriptor];
            m_hitGroupFunctionTable = [m_pipeline newIntersectionFunctionTableWithDescriptor:hitGroupFunctionTableDescriptor];
             
            // add functions to the dictionary
            auto intersectionFunctions = pipeline.TrinityALImpl_GetObject()->GetFunctionMap();
            auto hitGroupMap = pipeline.TrinityALImpl_GetObject()->GetHitGroupMap();
        
            int index = 0;
            
            // TODO: generalize so this can use the lambda function as well
            for( auto names : desc.m_missNames )
            {
                auto found = intersectionFunctions.find( names.name );
                // if we found nothing then the table is invalid
                if( found != end( intersectionFunctions ) )
                {
                    AddFunctionToVisibleTable( found->second, index++ );
                }
            }
            
            // reset the index for the hit group shader table
            index = 0;
            
            // a vector of offsets
            std::vector<uint64_t> ptrs;
            ptrs.reserve( desc.m_missNames.size() + desc.m_hitGroupNames.size() );
            
            auto FillTable = [&](const wchar_t* name, id <MTLFunction> fn, const Tr2RtLocalMaterialDescriptionAL& material, int index)->ALResult
            {
                if (@available(macOS 11.0, *))
                {                    
                    // Create a handle to the copy of the intersection function linked into the
                    // ray-tracing compute pipeline state. Create a different handle for each pipeline
                    // it is linked with.
                    id <MTLFunctionHandle> handle = [m_pipeline functionHandleWithFunction:fn];
                    
                    if( handle == nil )
                    {
                        return E_FAIL;
                    }
    
                    // Insert the handle into the intersection function table, which ultimately maps the
                    // geometry's index to its intersection function.
                    [m_hitGroupFunctionTable setFunction:handle atIndex:index];
                    
                    for( auto constant : material.m_constants )
                    {
                        auto& cb = constant;
                        if( cb.IsValid() )
                        {
                            uint64_t offset = renderContext.UploadConstants( cb );
                            
                            uint32_t page = uint32_t( offset >> 32 );
                            // have a vector of offsets (gpu address + addr) so for each instance it has it's own pointer into a large buffer.
                            // then bind that vector to the shadertable
                            id<MTLBuffer> cBuffer = renderContext.GetMetalContext()->GetConstantBufferAllocator().GetPage( uint32_t( page ) );
                            
                            renderContext.UseConstantBuffer(cBuffer);
                            
                            if( cBuffer.length != 0 )
                            {
                                if( @available(macOS 13.0, *) ) 
                                {
                                    ptrs.insert( ptrs.begin() + index, cBuffer.gpuAddress + offset );
                                }
                            }
                        }
                    }
                }
                
                return S_OK;
            };
            
            // Map each piece of scene hit group to its intersection function.
            for( auto hitGroup : desc.m_hitGroupNames )
            {
                auto found = hitGroupMap.find( hitGroup.name );
                
                // if hit group is found, then get the function name (ClosestHit)
                if( found != end( hitGroupMap ) )
                {
                    if( found->second.anyHit )
                    {
                        CR_RETURN_HR( FillTable( found->first.c_str(), found->second.anyHit, hitGroup.material, index++ ) );
                    }
                    if( found->second.intersection )
                    {
                        CR_RETURN_HR( FillTable( found->first.c_str(), found->second.intersection, hitGroup.material, index++ ) );
                    }
                    if( found->second.closestHit )
                    {
                        CR_RETURN_HR( FillTable( found->first.c_str(), found->second.closestHit, hitGroup.material, index++ ) );
                    }
                }
                else
                {
                    return E_FAIL;
                }
            }
            
            if( ptrs.size() != 0 )
            {
                id<MTLDevice> device = renderContext.GetMetalContext()->GetDevice();
                
                id<MTLBuffer> cBuffer = [device newBufferWithBytes: ptrs.data()
                                                length:ptrs.size() * sizeof(uint64_t)
                                                options:MTLResourceStorageModeShared];
                
                [m_hitGroupFunctionTable setBuffer:cBuffer offset:0 atIndex:0];
                
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

    void Tr2RtShaderTableAL::AddFunctionToIntersectionTable( id <MTLFunction> fn, const Tr2RtLocalMaterialDescriptionAL& material, int index )
    {
        if (@available(macOS 11.0, *)) 
        {
            // Create a handle to the copy of the intersection function linked into the
            // ray-tracing compute pipeline state. Create a different handle for each pipeline
            // it is linked with.
            id <MTLFunctionHandle> handle = [m_pipeline functionHandleWithFunction:fn];
            
            if( handle == nil )
            {
                return;
            }
            
            auto m = material;
            // Insert the handle into the intersection function table, which ultimately maps the
            // geometry's index to its intersection function.
            [m_hitGroupFunctionTable setFunction:handle atIndex:index];
        }
    }


    void Tr2RtShaderTableAL::AddFunctionToVisibleTable( id <MTLFunction> fn, int index )
    {
        if (@available(macOS 11.0, *))
        {
            // Create a handle to the copy of the intersection function linked into the
            // ray-tracing compute pipeline state. Create a different handle for each pipeline
            // it is linked with.
            id <MTLFunctionHandle> handle = [m_pipeline functionHandleWithFunction:fn];
            
            if( handle == nil )
            {
                return;
            }
            // Insert the handle into the intersection function table, which ultimately maps the
            // geometry's index to its intersection function.
            [m_missShaderFunctionTable setFunction:handle atIndex:index];
        }
    }

    void Tr2RtShaderTableAL::Destroy()
    {
        if( @available(macOS 11.0, *) )
        {
            m_hitGroupFunctionTable = nullptr;
            m_missShaderFunctionTable = nullptr;
        }
        m_pipeline = nullptr;
    }

    bool Tr2RtShaderTableAL::IsValid() const
    {
        if( @available(macOS 11.0, *) )
        {
            return m_hitGroupFunctionTable != nullptr || m_missShaderFunctionTable != nullptr;
        }
        return false;
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
