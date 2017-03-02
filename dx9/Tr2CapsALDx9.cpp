#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )
#include "Tr2CapsALDx9.h"

Tr2CapsAL::Tr2CapsAL()
{
	memset( &m_caps, 0, sizeof( m_caps ) );
}

ALResult Tr2CapsAL::QueryCaps( IDirect3DDevice9* device )
{
	memset( &m_caps, 0, sizeof( m_caps ) );
	return device->GetDeviceCaps( &m_caps );
}

bool Tr2CapsAL::SupportsFloat16() const
{
	const DWORD float16bits = D3DDTCAPS_FLOAT16_2 | D3DDTCAPS_FLOAT16_4;
	return ( m_caps.DeclTypes & float16bits ) == float16bits;
}

bool Tr2CapsAL::SupportsGpuBuffer() const
{
	return false;
}

bool Tr2CapsAL::SupportsStandaloneSwapChain() const
{
	return true;
}

uint32_t Tr2CapsAL::GetShaderVersion() const
{
	return ( m_caps.PixelShaderVersion >> 8 ) & 0x000000ff;
}

bool Tr2CapsAL::SupportsVertexShaderTextures() const
{
	return m_caps.VertexTextureFilterCaps != 0;
}

bool Tr2CapsAL::SupportsNoOverwriteLock() const
{
	return true;
}

#endif
