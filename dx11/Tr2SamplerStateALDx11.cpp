#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2SamplerStateALDx11.h"
#include "Tr2PrimaryRenderContextDx11.h"
#include "Tr2HalHelperStructures.h"

using namespace Tr2RenderContextEnum;

extern uint32_t g_forceAnisotropy;

namespace TrinityALImpl
{
	Tr2SamplerStateAL::Tr2SamplerStateAL()
	{
	}

	ALResult Tr2SamplerStateAL::Create( const Tr2SamplerDescription& description, Tr2PrimaryRenderContextAL &renderContext )
	{
		if( !renderContext.m_d3dDevice11 )
		{
			return E_FAIL;
		}

		D3D11_SAMPLER_DESC samplerDesc;
		if( g_forceAnisotropy != 1 && ( description.m_minFilter == TF_ANISOTROPIC || description.m_magFilter == TF_ANISOTROPIC || description.m_mipFilter == TF_ANISOTROPIC ) )
		{
			samplerDesc.Filter = D3D11_ENCODE_ANISOTROPIC_FILTER( description.m_isComparisonFilter ? 1 : 0 );
		}
		else
		{
			samplerDesc.Filter = D3D11_ENCODE_BASIC_FILTER(
				description.m_minFilter == TF_POINT ? D3D11_FILTER_TYPE_POINT : D3D11_FILTER_TYPE_LINEAR,
				description.m_magFilter == TF_POINT ? D3D11_FILTER_TYPE_POINT : D3D11_FILTER_TYPE_LINEAR,
				description.m_mipFilter == TF_POINT || description.m_mipFilter == TF_NONE ? D3D11_FILTER_TYPE_POINT : D3D11_FILTER_TYPE_LINEAR,
				description.m_isComparisonFilter ? 1 : 0 );
		}
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MODE( description.m_addressU );
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MODE( description.m_addressV );
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MODE( description.m_addressW );
		samplerDesc.MipLODBias = description.m_mipLODBias;
		samplerDesc.MaxAnisotropy = g_forceAnisotropy ? g_forceAnisotropy : description.m_maxAnisotropy;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_FUNC( description.m_comparisonFunc );
		samplerDesc.BorderColor[0] = description.m_borderColor[0];
		samplerDesc.BorderColor[1] = description.m_borderColor[1];
		samplerDesc.BorderColor[2] = description.m_borderColor[2];
		samplerDesc.BorderColor[3] = description.m_borderColor[3];
		samplerDesc.MinLOD = description.m_minLOD;
		samplerDesc.MaxLOD = description.m_mipFilter == TF_NONE ? description.m_minLOD : description.m_maxLOD;

		m_samplerState = nullptr;
		return renderContext.m_d3dDevice11->CreateSamplerState( &samplerDesc, &m_samplerState );
	}

	void Tr2SamplerStateAL::Destroy()
	{
		m_samplerState = nullptr;
	}

	bool Tr2SamplerStateAL::IsValid() const
	{
		return m_samplerState != nullptr;
	}

	Tr2ALMemoryType Tr2SamplerStateAL::GetMemoryClass() const 
	{
		return AL_MEMORY_MANAGED;
	}

	void Tr2SamplerStateAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2SamplerStateAL";
		description["name"] = m_name;
	}

	ALResult Tr2SamplerStateAL::SetName( const char* name )
	{
		m_name = name;
		SetDebugName( m_samplerState, name );
		return S_OK;
	}
}

#endif