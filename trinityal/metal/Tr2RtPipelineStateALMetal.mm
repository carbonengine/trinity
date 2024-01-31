//
//  Tr2RtPipelineStateALMetal.cpp
//  ShaderCompiler
//
//  Created by Iris Dogg Skarphedinsdottir on 4.1.2024.
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_METAL

#include "Tr2RtPipelineStateALMetal.h"
#include "Tr2PrimaryRenderContextMetal.h"
#include "../ALLog.h"
#include "MetalShaderStrings.h"


namespace TrinityALImpl
{
    Tr2RtPipelineStateAL::Tr2RtPipelineStateAL()
    {
    }

    Tr2RtPipelineStateAL::~Tr2RtPipelineStateAL()
    {
    }


    ALResult Tr2RtPipelineStateAL::CreateRtPipelineState( const Tr2RtPipelineStateDescriptionAL& desc, Tr2PrimaryRenderContextAL& renderContext )
    {
        m_shaderProgram = desc.m_shaderProgram;
        
        id<MTLDevice> device = renderContext.GetMetalContext()->GetDevice();
        // Maps intersection function names to actual MTLFunctions.
        
        // Fill out a dictionary of function constant values.
        MTLFunctionConstantValues *constants = [[MTLFunctionConstantValues alloc] init];
        // Each intersection function has its own set of resources. Determine the maximum size over all
        // intersection functions. This size becomes the stride that intersection functions use to find
        // the starting address for their resources.
        
        
        
        //id <MTLFunction> shadowFunction = [self specializedFunctionWithName:@"shadowRaytracingKernel"];
        MTLComputePipelineDescriptor *descriptor = [[MTLComputePipelineDescriptor alloc] init];
        
        
        // Set to YES to allow compiler to make certain optimizations
        descriptor.threadGroupSizeIsMultipleOfThreadExecutionWidth = YES;
        
        NSError *error = nullptr;
    
        descriptor.computeFunction = m_shaderProgram.TrinityALImpl_GetObject()->GetComputeKernel();
        
        // Create compute pipelines will execute code on the GPU
        // Create the compute pipeline state which does all the raytracing
       
        m_shadowPipeline = [device newComputePipelineStateWithDescriptor:descriptor
                                                                 options:0
                                                              reflection:nil
                                                                   error:&error];
        
        if( !m_shadowPipeline )
        {
            CCP_LOGERR("SOMETHING WENT WRONG WITH CREATING THE SHADOW PIPELINE FOR RAYTRACING");
        }
        
        
        return S_OK;
    }

    const ::Tr2ShaderProgramAL& Tr2RtPipelineStateAL::GetShaderProgram() const
    {
        return m_shaderProgram;
    }

    bool Tr2RtPipelineStateAL::IsValid() const
    {
        return true;
    }

    Tr2ALMemoryType Tr2RtPipelineStateAL::GetMemoryClass() const
    {
        return AL_MEMORY_MANAGED;
    }

    void Tr2RtPipelineStateAL::Destroy()
    {
        
    }
    
    void Tr2RtPipelineStateAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
    {
        description["type"] = "Tr2RtPipelineStateAL";
    }
/*
    const TrinityALImpl::Tr2RootSignatureAL& Tr2RtPipelineStateAL::GetGlobalRootSignature() const
    {
        return m_globalSignature;
    }

    ALResult Tr2RtPipelineStateAL::CreateRootSignature( TrinityALImpl::Tr2RootSignatureAL& rootSignature, const Tr2ShaderSignatureAL& signature, Tr2PrimaryRenderContextAL& renderContext )
    {
        return S_OK;
    }*/
}

#endif
