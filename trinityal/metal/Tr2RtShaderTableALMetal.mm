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
/*
namespace
{
    uintptr_t Align( uintptr_t offset, size_t alignment )
    {
        return (offset + (alignment - 1)) & ~(alignment - 1);
    }

    size_t GetSignatureSize( const TrinityALImpl::Tr2RootSignatureAL* signature )
    {
        if( !signature )
        {
            return 0;
        }
        size_t size = signature->m_cbRegisters.size() * sizeof( D3D12_GPU_DESCRIPTOR_HANDLE );
        if( signature->m_samplerTableSize )
        {
            size += sizeof( D3D12_GPU_DESCRIPTOR_HANDLE );
        }
        return size;
    }

    size_t GetMaxElementSize( const ::Tr2RtPipelineStateAL& pipeline )
    {
        size_t signatureSize = 0;
        auto& localSignatures = pipeline.TrinityALImpl_GetObject()->GetLocalSignatures();
        for( auto it = begin( localSignatures ); it != end( localSignatures ); ++it )
        {
            signatureSize = std::max( signatureSize, GetSignatureSize( *it ) );
        }
        return signatureSize;
    }
}*/


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
        }
            
        // add functions to the dictionary
        // in shaderTable for each hit group map into the dict and get index -> add to handle
        // then in TLAS the index would be the same somehow???
        auto intersectionFunctions = pipeline.TrinityALImpl_GetObject()->GetFunctionMap();
        auto hitGroupMap = pipeline.TrinityALImpl_GetObject()->GetHitGroupMap();
        auto funcIndexMap = pipeline.TrinityALImpl_GetObject()->GetFunctionIndexMap();
        
        int index = 0;
        
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
        
        //Map each piece of scene hit group to its intersection function.
        for( auto names : desc.m_hitGroupNames )
        {
            auto found = hitGroupMap.find( names.name );
            // if hit group is found, then get the function name (ClosestHit)
            if( found != end( hitGroupMap ) )
            {
                if( found->second.anyHit )
                {
                    AddFunctionToIntersectionTable( found->second.anyHit, index++ );
                }
                if( found->second.intersection )
                {
                    AddFunctionToIntersectionTable( found->second.intersection, index++ );
                }
                if( found->second.closestHit )
                {
                    AddFunctionToIntersectionTable( found->second.closestHit, index++ );
                }
                index++;
            }
            else
            {
                return E_FAIL;
            }
        }

        return S_OK;
    }

    void Tr2RtShaderTableAL::AddFunctionToIntersectionTable( id <MTLFunction> fn, int index )
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
    {/*
        if( m_owner )
        {
            m_owner = nullptr;
            m_table = nullptr;
            m_desc = Tr2RtShaderTableDescriptionAL();
            m_entrySize = 0;
        }
    */
    }

    bool Tr2RtShaderTableAL::IsValid() const
    {
        return true;
        //return m_table != nullptr;
    }

    Tr2ALMemoryType Tr2RtShaderTableAL::GetMemoryClass() const
    {
        return AL_MEMORY_MANAGED;
    }

    void Tr2RtShaderTableAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
    {
        description["type"] = "Tr2RtShaderTableAL";
    }
/*
    uint64_t Tr2RtShaderTableAL::GetMissShaderTableSize() const
    {
        return GetEntrySize() * m_desc.m_missNames.size();
    }

    uint64_t Tr2RtShaderTableAL::GetHitGroupTableSize() const
    {
        return GetEntrySize() * m_desc.m_hitGroupNames.size();
    }*/
}


#endif
