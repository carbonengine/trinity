#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#include "Tr2SamplerStateALDx9.h"

// Force anisotropic filtering switch:
// 0 means use whatever is specified in the effect (default)
// 1 means turn off anisotropic filtering (fallback to linear)
// >1 - force max anisotropy to specified number
uint32_t g_forceAnisotropy = 0;

using namespace Tr2RenderContextEnum;

Tr2SamplerStateAL::Tr2SamplerStateAL()
	:m_isValid( false )
{
	m_states[D3DSAMP_ADDRESSU] = D3DTADDRESS_WRAP;
	m_states[D3DSAMP_ADDRESSV] = D3DTADDRESS_WRAP;
	m_states[D3DSAMP_ADDRESSW] = D3DTADDRESS_WRAP;
	m_states[D3DSAMP_BORDERCOLOR] = 0;
	m_states[D3DSAMP_MAGFILTER] = D3DTEXF_LINEAR;
	m_states[D3DSAMP_MINFILTER] = D3DTEXF_LINEAR;
	m_states[D3DSAMP_MIPFILTER] = D3DTEXF_LINEAR;
	m_states[D3DSAMP_MIPMAPLODBIAS] = 0;
	m_states[D3DSAMP_MAXMIPLEVEL] = 0;
	m_states[D3DSAMP_MAXANISOTROPY] = 4;
}

ALResult Tr2SamplerStateAL::Create( 
	Tr2RenderContextAL& /*renderContext*/,
	const Tr2SamplerDescription& description )
{
	m_states[D3DSAMP_MINFILTER] = g_forceAnisotropy == 1 ? TF_LINEAR : description.m_minFilter;
	m_states[D3DSAMP_MAGFILTER] = g_forceAnisotropy == 1 ? TF_LINEAR : description.m_magFilter;
	m_states[D3DSAMP_MIPFILTER] = description.m_mipFilter;
	m_states[D3DSAMP_ADDRESSU] = description.m_addressU;
	m_states[D3DSAMP_ADDRESSV] = description.m_addressV;
	m_states[D3DSAMP_ADDRESSW] = description.m_addressW;
	m_states[D3DSAMP_MIPMAPLODBIAS] = *(uint32_t*)&description.m_mipLODBias;
	m_states[D3DSAMP_MAXANISOTROPY] = g_forceAnisotropy ? g_forceAnisotropy : description.m_maxAnisotropy;
	m_states[D3DSAMP_MAXMIPLEVEL] = uint32_t( description.m_minLOD );
	m_states[D3DSAMP_BORDERCOLOR] = 
		( uint32_t( std::min( std::max( description.m_borderColor[3] * 255.f, 0.f ), 255.f ) ) << 24 ) |
		( uint32_t( std::min( std::max( description.m_borderColor[0] * 255.f, 0.f ), 255.f ) ) << 16 ) |
		( uint32_t( std::min( std::max( description.m_borderColor[1] * 255.f, 0.f ), 255.f ) ) << 8 ) |
		uint32_t( std::min( std::max( description.m_borderColor[2] * 255.f, 0.f ), 255.f ) );
	m_isValid = true;
	ChangeObjectId();
	return S_OK;
}

bool Tr2SamplerStateAL::IsValid() const
{
	return m_isValid;
}

bool Tr2SamplerStateAL::operator==( const Tr2SamplerStateAL& sampler ) const
{
	return this == &sampler;
}

#endif // TRINITY_PLATFORM==TRINITY_DIRECTX9