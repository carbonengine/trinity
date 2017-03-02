#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "Tr2LockedRenderTargetALStub.h"

Tr2LockedRenderTargetAL::Tr2LockedRenderTargetAL()
	:m_pitch( 0 ),
	m_isLocked( false )
{
}

Tr2LockedRenderTargetAL::~Tr2LockedRenderTargetAL()
{
	Destroy();
}

void Tr2LockedRenderTargetAL::Destroy()
{
	m_data.clear();
	m_pitch = 0;
}

bool Tr2LockedRenderTargetAL::IsValid() const
{
	return m_pitch != 0;
}

ALResult Tr2LockedRenderTargetAL::Lock( void*& data, uint32_t& pitch, Tr2RenderContextAL& )
{
	if( m_hasLockedRect )
	{
		pitch = m_pitch;
		data = m_data.get();
		if( !data )
		{
			return E_FAIL;
		}
	}
	m_isLocked = true;
	return S_OK;
}

ALResult Tr2LockedRenderTargetAL::Unlock( Tr2RenderContextAL& )
{
	if( !m_isLocked )
	{
		return E_INVALIDCALL;
	}
	m_isLocked = false;
	return S_OK;
}

#endif