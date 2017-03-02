#pragma once
#ifndef Tr2LockedRenderTargetALGLES2_H
#define Tr2LockedRenderTargetALGLES2_H

#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )

class Tr2LockedRenderTargetAL
{
public:
	Tr2LockedRenderTargetAL();
	~Tr2LockedRenderTargetAL();

	void Destroy();
	bool IsValid() const;

	ALResult Lock( void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext );
	ALResult Unlock( Tr2RenderContextAL& renderContext );
private:
	Tr2LockedRenderTargetAL( const Tr2LockedRenderTargetAL& ) /* = delete */;
	Tr2LockedRenderTargetAL& operator=( const Tr2LockedRenderTargetAL& ) /* = delete */;

	friend class Tr2RenderTargetAL;

	CcpMallocBuffer m_lockedData;
	uint32_t m_pitch;
};

#endif

#endif