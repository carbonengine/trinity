#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "Tr2LockedRenderTargetALGLES2.h"

Tr2LockedRenderTargetAL::Tr2LockedRenderTargetAL()
	:m_pitch( 0 )
{
}

Tr2LockedRenderTargetAL::~Tr2LockedRenderTargetAL()
{
}

void Tr2LockedRenderTargetAL::Destroy()
{
	m_lockedData.clear();
	m_pitch = 0;
}

bool Tr2LockedRenderTargetAL::IsValid() const
{
	return m_pitch != 0;
}

ALResult Tr2LockedRenderTargetAL::Lock( void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext )
{
	if( !IsValid() || !renderContext.IsValid() )
	{
		return E_FAIL;
	}
	data = m_lockedData.get();
	pitch = m_pitch;
    return S_OK;
}

ALResult Tr2LockedRenderTargetAL::Unlock( Tr2RenderContextAL& renderContext )
{
	if( !IsValid() || !renderContext.IsValid() )
	{
		return E_FAIL;
	}
	return S_OK;
}

#endif