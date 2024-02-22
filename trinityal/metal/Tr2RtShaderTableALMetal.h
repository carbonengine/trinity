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
        id <MTLIntersectionFunctionTable> GetIntersectionFunctionTable() { return m_intersectionFunctionTable; }
        /*uint64_t GetEntrySize() const;
        uint64_t GetMissShaderTableSize() const;
        uint64_t GetHitGroupTableSize() const;
         */
    private:
        void AddFunctionToTable( id <MTLFunction> fn, int index );
        Tr2RtShaderTableDescriptionAL m_desc;
        //uint64_t m_entrySize;
        id <MTLIntersectionFunctionTable> m_intersectionFunctionTable;
        id <MTLComputePipelineState> m_pipeline;
        
    };
}

#endif
