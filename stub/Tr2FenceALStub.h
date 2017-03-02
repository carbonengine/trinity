#pragma once
#ifndef Tr2FenceALStub_H
#define Tr2FenceALStub_H

#if( TRINITY_PLATFORM==TRINITY_STUB )

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

	bool operator==( const Tr2FenceAL& other ) const
	{
		return this == &other;
	}

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_VIDEO; }

private:
	bool m_isValid;
	bool m_hasFence;
};

#endif

#endif