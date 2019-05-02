////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"


class Tr2PrimaryRenderContextAL;
class Tr2RenderContextAL;


class Tr2FenceAL : 
	public Tr2TrackedALObject<Tr2RenderContextEnum::OT_FENCE>
{
public:
	Tr2FenceAL()
	{

	}
	~Tr2FenceAL()
	{

	}

	ALResult Create( Tr2PrimaryRenderContextAL& renderContext )
	{
		return E_NOTIMPL;
	}
	void Destroy()
	{

	}

	bool IsValid() const
	{
		return false;
	}

	ALResult PutFence( Tr2RenderContextAL& renderContext )
	{
		return E_NOTIMPL;
	}
	ALResult IsReached( bool& isReached, Tr2RenderContextAL& renderContext )
	{
		return E_NOTIMPL;
	}
	ALResult Wait( Tr2RenderContextAL& renderContext )
	{
		return E_NOTIMPL;
	}

	bool operator==( const Tr2FenceAL& other ) const
	{
		return this == &other;
	}

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_VIDEO; }
private:
	Tr2FenceAL( const Tr2FenceAL& ) /* = delete */;
	Tr2FenceAL& operator=( const Tr2FenceAL& ) /* = delete */;
};

#endif
