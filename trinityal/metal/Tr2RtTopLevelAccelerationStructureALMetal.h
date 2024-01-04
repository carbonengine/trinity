//
//  Tr2RtTopLevelAccelerationStructureALMetal.h

//  carbon-trinity
//
//  Created by Iris Dogg Skarphedinsdottir on 3.1.2024.
//


#pragma once

#if TRINITY_PLATFORM == TRINITY_METAL

#include "../include/Tr2RtTopLevelAccelerationStructureAL.h"


namespace TrinityALImpl
{
    class Tr2RtTopLevelAccelerationStructureAL : public Tr2DeviceResourceAL<Tr2RtTopLevelAccelerationStructureAL>
    {
    public:
        Tr2RtTopLevelAccelerationStructureAL();
        ~Tr2RtTopLevelAccelerationStructureAL();

        ALResult Create( const size_t count, const Tr2RtInstanceAL* instances, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext );
        ALResult Update( const size_t count, const Tr2RtInstanceAL* instances, Tr2RenderContextAL& renderContext );

        bool IsValid() const;

        void Destroy();
        Tr2ALMemoryType GetMemoryClass() const;
        void Describe( Tr2DeviceResourceDescriptionAL& description ) const;

        const ::Tr2BufferAL& GetBuffer() const;
        size_t GetCapacity() const;
        
    private:
        ::Tr2BufferAL m_buffer;

    };
}

#endif
