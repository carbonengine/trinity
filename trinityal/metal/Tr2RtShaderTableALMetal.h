//
//  Tr2RtShaderTableALMetal.hpp
//  ShaderCompiler
//
//  Created by Iris Dogg Skarphedinsdottir on 4.1.2024.
//


#pragma once

#if TRINITY_PLATFORM == TRINITY_METAL

#include <Metal/Metal.h>
#include "../include/Tr2RtShaderTableAL.h"
#include "../include/Tr2RtPipelineStateAL.h"

namespace TrinityALImpl
{
    class Tr2RtShaderTableAL : public Tr2DeviceResourceAL<Tr2RtShaderTableAL>
    {
    public:
        Tr2RtShaderTableAL();
        ~Tr2RtShaderTableAL();

        ALResult Create( const Tr2RtShaderTableDescriptionAL& desc, const ::Tr2RtPipelineStateAL& pipeline, Tr2PrimaryRenderContextAL& renderContext );
        void Destroy();
        bool IsValid() const;

        Tr2ALMemoryType GetMemoryClass() const;
        void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
        
        API_AVAILABLE( macos(11.0) )
        id <MTLIntersectionFunctionTable> GetHitGroupFunctionTable() { return m_hitGroupFunctionTable; }
        
        API_AVAILABLE( macos(11.0) )
        id <MTLVisibleFunctionTable> GetMissShaderFunctionTable() { return m_missShaderFunctionTable; }

    private:
        void AddFunctionToIntersectionTable( id <MTLFunction> fn, const Tr2RtLocalMaterialDescriptionAL& material, int index );
        void AddFunctionToVisibleTable( id <MTLFunction> fn, int index );
        Tr2RtShaderTableDescriptionAL m_desc;

        API_AVAILABLE( macos(11.0) )
        id <MTLIntersectionFunctionTable> m_hitGroupFunctionTable;
        
        API_AVAILABLE( macos(11.0) )
        id <MTLVisibleFunctionTable> m_missShaderFunctionTable;
        
        id <MTLComputePipelineState> m_pipeline;
        
    };
}

#endif
