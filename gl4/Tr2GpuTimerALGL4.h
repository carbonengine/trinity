#pragma once
#ifndef Tr2GpuTimerALGLES2_H
#define Tr2GpuTimerALGLES2_H

#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )

class Tr2GpuTimerAL: 
	public Tr2TrackedALObject<Tr2RenderContextEnum::OT_TIMER>,
	public Tr2AutoResetObjectAL
{
public:
	Tr2GpuTimerAL();

	ALResult Create( Tr2PrimaryRenderContextAL& renderContext );
	void Destroy();

	bool Begin( Tr2RenderContextAL& renderContext );
	void End( Tr2RenderContextAL& renderContext );

	float GetTime( Tr2RenderContextAL& renderContext );

	bool IsValid() const;

	bool operator==( const Tr2GpuTimerAL& other ) const
	{
		return this == &other;
	}

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_VIDEO; }

	virtual void ReleaseALResource();
	virtual void PrepareALResource( Tr2PrimaryRenderContextAL& renderContext );
private:
};

#endif

#endif
