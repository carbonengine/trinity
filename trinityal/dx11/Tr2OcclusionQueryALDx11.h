// Copyright © 2023 CCP ehf.

#pragma once

#if ( TRINITY_PLATFORM == TRINITY_DIRECTX11 )

#include "../include/Tr2OcclusionQueryAL.h"

namespace TrinityALImpl
{
// -------------------------------------------------------------
// Description:
//  Wraps the hardware specifics of running an occlusion query.
//  32bit - we do not support returning a query of > 4 gig pixels
// -------------------------------------------------------------
class Tr2OcclusionQueryAL : public Tr2DeviceResourceAL<Tr2OcclusionQueryAL>
{
public:
	Tr2OcclusionQueryAL();

	ALResult Create( Tr2PrimaryRenderContextAL& renderContext );
	bool IsValid() const;
	void Destroy();

	ALResult Begin( Tr2RenderContextAL& renderContext );
	ALResult End( Tr2RenderContextAL& renderContext );
	ALResult GetPixelCount( Tr2RenderContextAL& renderContext, uint32_t& count, ::Tr2OcclusionQueryAL::WaitMode waitMode );

	Tr2ALMemoryType GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}
	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	ALResult SetName( const char* name );

private:
	Tr2OcclusionQueryAL( const Tr2OcclusionQueryAL& ) /* = delete */;
	Tr2OcclusionQueryAL& operator=( const Tr2OcclusionQueryAL& ) /* = delete */;

	CComPtr<ID3D11Query> m_query;
	std::string m_name;
};
}

#endif
