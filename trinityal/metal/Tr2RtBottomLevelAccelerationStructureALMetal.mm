//
//  Tr2RtBottomLevelAccelerationStructureMetal.m
//  carbon-trinity
//
//  Created by Iris Dogg Skarphedinsdottir on 3.1.2024.
//
#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_METAL

#include "Tr2RtBottomLevelAccelerationStructureALMetal.h"

namespace  TrinityALImpl {

    Tr2RtBottomLevelAccelerationStructureAL::Tr2RtBottomLevelAccelerationStructureAL()
    {
        
    }

    Tr2RtBottomLevelAccelerationStructureAL::~Tr2RtBottomLevelAccelerationStructureAL()
    {
        
    }

    ALResult Tr2RtBottomLevelAccelerationStructureAL::Create( const Tr2RtPositionStreamAL& positions, const Tr2RtIndicesStreamAL& indices, int numObjects, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext )
    {
        return S_OK;
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
