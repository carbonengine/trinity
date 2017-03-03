#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "Tr2LockedRenderTargetALStub.h"

Tr2LockedRenderTargetAL::Tr2LockedRenderTargetAL()
	:m_pitch( 0 )
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
	data = m_data.get();
	if( !data )
	{
		return E_FAIL;
	}
	pitch = m_pitch;
	return S_OK;
}

ALResult Tr2LockedRenderTargetAL::Unlock( Tr2RenderContextAL& )
{
	return S_OK;
}

#endif