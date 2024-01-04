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
#include "Tr2PrimaryRenderContextMetal.h"

namespace TrinityALImpl
{

    Tr2RtTopLevelAccelerationStructureAL::Tr2RtTopLevelAccelerationStructureAL()
    {
    }


    Tr2RtTopLevelAccelerationStructureAL::~Tr2RtTopLevelAccelerationStructureAL()
    {
    }

    ALResult Tr2RtTopLevelAccelerationStructureAL::Create( size_t count, const Tr2RtInstanceAL* instances, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext )
    {
        return E_FAIL;
    }

    ALResult Tr2RtTopLevelAccelerationStructureAL::Update( size_t count, const Tr2RtInstanceAL* instances, Tr2RenderContextAL& renderContext )
    {
        return E_FAIL;
    }

    bool Tr2RtTopLevelAccelerationStructureAL::IsValid() const
    {
        return false;
    }
    
    const ::Tr2BufferAL& Tr2RtTopLevelAccelerationStructureAL::GetBuffer() const
    {
        return m_buffer;
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
