#pragma once
#ifndef Tr2LockedRenderTargetALStub_H
#define Tr2LockedRenderTargetALStub_H


#include "../ALResult.h"


class Tr2RenderContextAL;


#if( TRINITY_PLATFORM==TRINITY_STUB )

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
	uint32_t m_pitch;
	CcpMallocBuffer m_data;
};

#endif

#endif