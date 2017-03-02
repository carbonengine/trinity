#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2LockedRenderTargetALDx11.h"

Tr2LockedRenderTargetAL::Tr2LockedRenderTargetAL()
{
}

Tr2LockedRenderTargetAL::~Tr2LockedRenderTargetAL()
{
}

void Tr2LockedRenderTargetAL::Destroy()
{
	m_staging = nullptr;
}

bool Tr2LockedRenderTargetAL::IsValid() const
{
	return m_staging != nullptr;
}

ALResult Tr2LockedRenderTargetAL::Lock( void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext )
{
	if( !IsValid() || !renderContext.IsValid() || !m_staging )
	{
		return E_FAIL;
	}
	D3D11_MAPPED_SUBRESOURCE ms = { nullptr, 0, 0 };
	HRESULT hr = renderContext.m_context->Map( m_staging, 0, D3D11_MAP_READ, 0, &ms );
	data = ms.pData;
	pitch = ms.RowPitch;
	if( !ms.pData )
	{
		return E_FAIL;
	}
    return hr;
}

ALResult Tr2LockedRenderTargetAL::Unlock( Tr2RenderContextAL& renderContext )
{
	if( !IsValid() || !renderContext.IsValid() || !m_staging )
	{
		return E_FAIL;
	}
	renderContext.m_context->Unmap( m_staging, 0 );
	return S_OK;
}

#endif