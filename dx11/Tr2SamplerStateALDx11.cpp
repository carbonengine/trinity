#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2SamplerStateALDx11.h"

// Force anisotropic filtering switch:
// 0 means use whatever is specified in the effect (default)
// 1 means turn off anisotropic filtering (fallback to linear)
// >1 - force max anisotropy to specified number
uint32_t g_forceAnisotropy = 0;

using namespace Tr2RenderContextEnum;

Tr2SamplerStateAL::Tr2SamplerStateAL()
{
	// These default values are slightly different from DX11
	// defaults, but are the same as DX9 trinity defaults.
	m_samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	m_samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	m_samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	m_samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	m_samplerDesc.MinLOD = -FLT_MAX;
	m_samplerDesc.MaxLOD = FLT_MAX;
	m_samplerDesc.MipLODBias = 0;
	m_samplerDesc.MaxAnisotropy = 4;
	m_samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	m_samplerDesc.BorderColor[0] = 0;
	m_samplerDesc.BorderColor[1] = 0;
	m_samplerDesc.BorderColor[2] = 0;
	m_samplerDesc.BorderColor[3] = 0;
}

ALResult Tr2SamplerStateAL::Create( 
	Tr2PrimaryRenderContextAL &renderContext,
	const Tr2SamplerDescription& description )
{
	if( !renderContext.m_d3dDevice11 )
	{
		return E_FAIL;
	}

	if( g_forceAnisotropy != 1 && ( description.m_minFilter == TF_ANISOTROPIC || description.m_magFilter == TF_ANISOTROPIC || description.m_mipFilter == TF_ANISOTROPIC ) )
	{
		m_samplerDesc.Filter = D3D11_ENCODE_ANISOTROPIC_FILTER( description.m_isComparisonFilter );
	}
	else
	{
		m_samplerDesc.Filter = D3D11_ENCODE_BASIC_FILTER( 
			description.m_minFilter == TF_POINT ? D3D11_FILTER_TYPE_POINT : D3D11_FILTER_TYPE_LINEAR, 
			description.m_magFilter == TF_POINT ? D3D11_FILTER_TYPE_POINT : D3D11_FILTER_TYPE_LINEAR, 
			description.m_mipFilter == TF_POINT || description.m_mipFilter == TF_NONE ? D3D11_FILTER_TYPE_POINT : D3D11_FILTER_TYPE_LINEAR, 
			description.m_isComparisonFilter );
	}
	m_samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MODE( description.m_addressU );
	m_samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MODE( description.m_addressV );
	m_samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MODE( description.m_addressW );
	m_samplerDesc.MipLODBias = description.m_mipLODBias;
	m_samplerDesc.MaxAnisotropy = g_forceAnisotropy ? g_forceAnisotropy : description.m_maxAnisotropy;
	m_samplerDesc.ComparisonFunc = D3D11_COMPARISON_FUNC( description.m_comparisonFunc );
	m_samplerDesc.BorderColor[0] = description.m_borderColor[0];
	m_samplerDesc.BorderColor[1] = description.m_borderColor[1];
	m_samplerDesc.BorderColor[2] = description.m_borderColor[2];
	m_samplerDesc.BorderColor[3] = description.m_borderColor[3];
	m_samplerDesc.MinLOD = description.m_minLOD;
	m_samplerDesc.MaxLOD = description.m_mipFilter == TF_NONE ? description.m_minLOD : description.m_maxLOD;

	m_samplerState = nullptr;
	HRESULT hr = renderContext.m_d3dDevice11->CreateSamplerState( &m_samplerDesc, &m_samplerState );
	if( SUCCEEDED( hr ) )
	{
		ChangeObjectId();
	}
	return hr;
}

bool Tr2SamplerStateAL::operator==( const Tr2SamplerStateAL& sampler ) const
{
	return this == &sampler;
}

#endif // TRINITY_PLATFORM==TRINITY_DIRECTX11