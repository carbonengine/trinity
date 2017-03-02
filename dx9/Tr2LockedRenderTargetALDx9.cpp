#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#include "Tr2LockedRenderTargetALDx9.h"

Tr2LockedRenderTargetAL::Tr2LockedRenderTargetAL()
{
}

Tr2LockedRenderTargetAL::~Tr2LockedRenderTargetAL()
{
}

void Tr2LockedRenderTargetAL::Destroy()
{
	m_sysMemLocked = nullptr;
}

bool Tr2LockedRenderTargetAL::IsValid() const
{
	return m_sysMemLocked != nullptr;
}

ALResult Tr2LockedRenderTargetAL::Lock( void*& data, uint32_t& pitch, Tr2RenderContextAL& )
{
	D3DLOCKED_RECT locked;
	if( m_hasLockedRect )
	{
		RECT R = { m_lockedRect[0], m_lockedRect[1], m_lockedRect[2], m_lockedRect[3] };
		CR_RETURN_HR( m_sysMemLocked->LockRect( &locked, &R, D3DLOCK_READONLY ) );
	}
	else
	{
		CR_RETURN_HR( m_sysMemLocked->LockRect( &locked, NULL, D3DLOCK_READONLY ) );
	}
		
	data  = locked.pBits;
	pitch = locked.Pitch;

	if( data )
	{
		return S_OK;
	}
	return E_FAIL;
}

ALResult Tr2LockedRenderTargetAL::Unlock( Tr2RenderContextAL& )
{
	return m_sysMemLocked->UnlockRect();
}

#endif