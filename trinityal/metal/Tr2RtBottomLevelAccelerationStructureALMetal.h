// Copyright © 2024 CCP ehf.

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

	ALResult Create( const Tr2RtGeometryAL& geometry,
					 const Tr2RtGeometryAL& capacity,
					 Tr2RtBlasGeometryFlags::Type geometryFlags,
					 Tr2RtBuildFlags::Type buildFlags,
					 Tr2PrimaryRenderContextAL& renderContext );
	ALResult Update( const Tr2RtGeometryAL& geometry, Tr2RenderContextAL& renderContext );
	ALResult CompactBlas( Tr2PrimaryRenderContextAL& renderContext );
	bool IsValid() const;

	void Destroy();
	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	Tr2ALMemoryType GetMemoryClass() const;

	API_AVAILABLE( macos( 11.0 ) )
	id<MTLAccelerationStructure> m_primitiveAccelerationStructure;

private:
	API_AVAILABLE( macos( 11.0 ) )
	MTLAccelerationStructureTriangleGeometryDescriptor* m_geomDesc;

	API_AVAILABLE( macos( 11.0 ) )
	MTLPrimitiveAccelerationStructureDescriptor* m_accelerationStructureDesc;

	NSUInteger m_scratchBufferSize;

	id<MTLBuffer> m_scratchBuffer;
};
}

#endif
