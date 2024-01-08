//
//  Tr2RtBottomLevelAccelerationStructureALMetal.h
//  carbon-trinity
//
//  Created by Iris Dogg Skarphedinsdottir on 3.1.2024.
//


// NOTE: on DXR we have these things called BottomLevelAccelerationStructure (BLAS) and TopLevelAccelerationStructure (TLAS). 
// In Metal similar objects are referenced as primitive acceleration structure (BLAS) and instance acceleration structure (TLAS)

#pragma once

#if TRINITY_PLATFORM == TRINITY_METAL

#include "../include/Tr2RtBottomLevelAccelerationStructureAL.h"
#include "Tr2PrimaryRenderContextAL.h"

namespace TrinityALImpl
{
    class Tr2RtBottomLevelAccelerationStructureAL : public Tr2DeviceResourceAL<Tr2RtBottomLevelAccelerationStructureAL>
    {
    public:
        Tr2RtBottomLevelAccelerationStructureAL();
        ~Tr2RtBottomLevelAccelerationStructureAL();
        
        ALResult Create( const Tr2RtPositionStreamAL& positions, const Tr2RtIndicesStreamAL& indices, int numObjects, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext );
        ALResult Update( const Tr2RtPositionStreamAL& positions, const Tr2RtIndicesStreamAL& indices, Tr2RenderContextAL& renderContext );
        ALResult CompactBlas( Tr2PrimaryRenderContextAL& renderContext );
        bool IsValid() const;
        
        void Destroy();
        void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
        Tr2ALMemoryType GetMemoryClass() const;
        
    private:
        //BLAS
        NSMutableArray *_primitiveAccelerationStructures;
    };
}

#endif
