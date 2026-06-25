// Copyright © 2023 CCP ehf.

#pragma once

#if ( TRINITY_PLATFORM == TRINITY_STUB )

#include "../include/Tr2OcclusionQueryAL.h"

namespace TrinityALImpl
{
class Tr2OcclusionQueryAL : public Tr2DeviceResourceAL<Tr2OcclusionQueryAL>
{
public:
	Tr2OcclusionQueryAL();
	~Tr2OcclusionQueryAL();

	ALResult Create( Tr2RenderContextAL& renderContext );
	bool IsValid() const;
	void Destroy();

	ALResult Begin( Tr2RenderContextAL& renderContext );
	ALResult End( Tr2RenderContextAL& renderContext );
	ALResult GetPixelCount( Tr2RenderContextAL& renderContext, uint32_t& count, ::Tr2OcclusionQueryAL::WaitMode waitMode );

	Tr2ALMemoryType GetMemoryClass() const
	{
		return AL_MEMORY_VIDEO;
	}
	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	ALResult SetName( const char* name );

private:
	bool m_isValid;
	bool m_isRunning;
	Tr2OcclusionQueryAL( const Tr2OcclusionQueryAL& ) /* = delete */;
	Tr2OcclusionQueryAL& operator=( const Tr2OcclusionQueryAL& ) /* = delete */;
};
}

#endif
