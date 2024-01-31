//
//  Tr2RtPipelineStateALMetal.hpp
//  ShaderCompiler
//
//  Created by Iris Dogg Skarphedinsdottir on 4.1.2024.
//
#pragma once

#if TRINITY_PLATFORM == TRINITY_METAL


//#include "StdAfx.h"
#include "../include/Tr2RtPipeLineStateAL.h"
#include "Tr2ShaderProgramALMetal.h"
#include <Metal/Metal.h>

namespace TrinityALImpl
{
    class Tr2RtPipelineStateAL : public Tr2DeviceResourceAL<Tr2RtPipelineStateAL>
    {
    public:
        Tr2RtPipelineStateAL();
        ~Tr2RtPipelineStateAL();

        ALResult CreateRtPipelineState( const Tr2RtPipelineStateDescriptionAL& desc, Tr2PrimaryRenderContextAL& renderContext );
        void Destroy();
        bool IsValid() const;

        Tr2ALMemoryType GetMemoryClass() const;
        void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
        id <MTLComputePipelineState> GetRtPipeline() { return m_shadowPipeline; }
        const ::Tr2ShaderProgramAL& GetShaderProgram() const;
        
        
    private:
        id <MTLComputePipelineState> m_shadowPipeline;
        ::Tr2ShaderProgramAL m_shaderProgram;
    };
}

#endif
