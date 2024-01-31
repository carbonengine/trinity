//
//  Tr2RtTopLevelAccelerationStructureALMetal.h

//  carbon-trinity
//
//  Created by Iris Dogg Skarphedinsdottir on 3.1.2024.
//


#pragma once

#if TRINITY_PLATFORM == TRINITY_METAL

#include "../include/Tr2RtTopLevelAccelerationStructureAL.h"
#include "Tr2RtBottomLevelAccelerationStructureALMetal.h"
#include <Metal/Metal.h>
#import <MetalKit/MetalKit.h>


namespace TrinityALImpl
{
class API_AVAILABLE(macos(11.0)) Tr2RtTopLevelAccelerationStructureAL : public Tr2DeviceResourceAL<Tr2RtTopLevelAccelerationStructureAL>
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
        NSMutableArray *GetPrimitiveAccelerationStructures(){ return m_primitiveAccelerationStructures; }
    private:
        id<MTLBuffer> m_instanceBuffer;
        
        ::Tr2BufferAL m_buffer;
        
        NSMutableArray* m_primitiveAccelerationStructures;
        id <MTLAccelerationStructure> m_instanceAccelerationStructure;
        
        id <MTLAccelerationStructure> BuildAccelerationStructure(MTLAccelerationStructureDescriptor* descriptor, id<MTLDevice> device,  MetalContext* metalContext );
        
    };
}

#endif
