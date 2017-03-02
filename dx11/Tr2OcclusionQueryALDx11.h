#pragma once
#ifndef Tr2OcclusionQueryALDx11_H
#define Tr2OcclusionQueryALDx11_H

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

// -------------------------------------------------------------
// Description:
//  Wraps the hardware specifics of running an occlusion query.
//  32bit - we do not support returning a query of > 4 gig pixels
// -------------------------------------------------------------
class Tr2OcclusionQueryAL: public Tr2TrackedALObject<Tr2RenderContextEnum::OT_OCCLUSION_QUERY>
{
public:
	enum WaitMode
	{
		WAIT,
		DO_NOT_WAIT,
	};

	Tr2OcclusionQueryAL() {}

	ALResult Create( Tr2PrimaryRenderContextAL& renderContext );
	bool IsValid() const;
	void Destroy();

	ALResult Begin( Tr2RenderContextAL& renderContext );
	ALResult End( Tr2RenderContextAL& renderContext );
	ALResult GetPixelCount( Tr2RenderContextAL& renderContext, uint32_t& count, WaitMode waitMode = DO_NOT_WAIT );

	bool operator==( const Tr2OcclusionQueryAL& other ) const { return m_query == other.m_query; }

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }
private:
	Tr2OcclusionQueryAL( const Tr2OcclusionQueryAL& ) /* = delete */;
	Tr2OcclusionQueryAL& operator=( const Tr2OcclusionQueryAL& ) /* = delete */;

	CComPtr<ID3D11Query> m_query;
};

#endif

#endif // Tr2OcclusionQueryALDx11_H