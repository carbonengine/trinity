#pragma once
#ifndef Tr2FenceALDx11_H
#define Tr2FenceALDx11_H

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

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
	Tr2FenceAL( const Tr2OcclusionQueryAL& ) /* = delete */;
	Tr2FenceAL& operator=( const Tr2OcclusionQueryAL& ) /* = delete */;

	CComPtr<ID3D11Query> m_query;
};

#endif

#endif