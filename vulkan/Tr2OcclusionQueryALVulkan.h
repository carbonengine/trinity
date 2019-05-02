////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_VULKAN

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

	Tr2OcclusionQueryAL() {}

	ALResult Create( Tr2PrimaryRenderContextAL& renderContext )
	{
		return E_NOTIMPL;
	}
	bool IsValid() const
	{
		return false;
	}
	void Destroy()
	{

	}

	ALResult Begin( Tr2RenderContextAL& renderContext )
	{
		return E_NOTIMPL;
	}
	ALResult End( Tr2RenderContextAL& renderContext )
	{
		return E_NOTIMPL;
	}
	ALResult GetPixelCount( Tr2RenderContextAL& renderContext, uint32_t& count, WaitMode waitMode = DO_NOT_WAIT )
	{
		return E_NOTIMPL;
	}

	bool operator==( const Tr2OcclusionQueryAL& other ) const { return false; }

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }
private:
	Tr2OcclusionQueryAL( const Tr2OcclusionQueryAL& ) /* = delete */;
	Tr2OcclusionQueryAL& operator=( const Tr2OcclusionQueryAL& ) /* = delete */;
};

#endif
