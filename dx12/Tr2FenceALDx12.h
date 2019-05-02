////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"


class Tr2PrimaryRenderContextAL;
class Tr2RenderContextAL;


class Tr2FenceAL : 
	public Tr2TrackedALObject<Tr2RenderContextEnum::OT_FENCE>
{
public:
	Tr2FenceAL();
	~Tr2FenceAL();

	ALResult Create( Tr2PrimaryRenderContextAL& renderContext );
	void Destroy();
	bool IsValid() const;

	ALResult PutFence( Tr2RenderContextAL& renderContext );
	ALResult IsReached( bool& isReached, Tr2RenderContextAL& renderContext );
	ALResult Wait( Tr2RenderContextAL& renderContext );

	bool operator==( const Tr2FenceAL& other ) const;
	Tr2ALMemoryType GetMemoryClass() const;
private:
	Tr2FenceAL( const Tr2FenceAL& ) /* = delete */;
	Tr2FenceAL& operator=( const Tr2FenceAL& ) /* = delete */;

	uint64_t m_frameIndex;
	Tr2PrimaryRenderContextAL* m_owner;
};

#endif
