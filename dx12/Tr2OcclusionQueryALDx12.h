////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../Tr2TrackedALObject.h"
#include "../ALResult.h"


class Tr2PrimaryRenderContextAL;
class Tr2RenderContextAL;


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

	Tr2OcclusionQueryAL();
	~Tr2OcclusionQueryAL();

	ALResult Create( Tr2PrimaryRenderContextAL& renderContext );
	void Destroy();

	bool IsValid() const;

	ALResult Begin( Tr2RenderContextAL& renderContext );
	ALResult End( Tr2RenderContextAL& renderContext );
	ALResult GetPixelCount( Tr2RenderContextAL& renderContext, uint32_t& count, WaitMode waitMode = DO_NOT_WAIT );

	bool operator==( const Tr2OcclusionQueryAL& other ) const;

	Tr2ALMemoryType GetMemoryClass() const;
private:
	Tr2OcclusionQueryAL( const Tr2OcclusionQueryAL& ) /* = delete */;
	Tr2OcclusionQueryAL& operator=( const Tr2OcclusionQueryAL& ) /* = delete */;

	CComPtr<ID3D12QueryHeap> m_query;
	CComPtr<ID3D12Resource> m_result;
	uint64_t m_frameIndex;
	Tr2PrimaryRenderContextAL* m_owner;
};

#endif
