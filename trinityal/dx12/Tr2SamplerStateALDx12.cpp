// Copyright © 2019 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2SamplerStateALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"

using namespace Tr2RenderContextEnum;

extern uint32_t g_forceAnisotropy;

namespace TrinityALImpl
{
Tr2SamplerStateAL::Tr2SamplerStateAL() :
	m_indexInHeap( 0xffffffff ),
	m_isValid( false )
{
}

ALResult Tr2SamplerStateAL::Create( const Tr2SamplerDescription& description, Tr2PrimaryRenderContextAL& renderContext )
{
	Destroy();
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}

	if( g_forceAnisotropy != 1 && ( description.m_minFilter == TF_ANISOTROPIC || description.m_magFilter == TF_ANISOTROPIC || description.m_mipFilter == TF_ANISOTROPIC ) )
	{
		m_sampler.Filter = D3D12_ENCODE_ANISOTROPIC_FILTER( description.m_isComparisonFilter ? 1 : 0 );
	}
	else
	{
		m_sampler.Filter = D3D12_ENCODE_BASIC_FILTER(
			description.m_minFilter == TF_POINT ? D3D12_FILTER_TYPE_POINT : D3D12_FILTER_TYPE_LINEAR,
			description.m_magFilter == TF_POINT ? D3D12_FILTER_TYPE_POINT : D3D12_FILTER_TYPE_LINEAR,
			description.m_mipFilter == TF_POINT || description.m_mipFilter == TF_NONE ? D3D12_FILTER_TYPE_POINT : D3D12_FILTER_TYPE_LINEAR,
			description.m_isComparisonFilter ? 1 : 0 );
	}
	m_sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE( description.m_addressU );
	m_sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE( description.m_addressV );
	m_sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE( description.m_addressW );
	m_sampler.MipLODBias = description.m_mipLODBias;
	m_sampler.MaxAnisotropy = g_forceAnisotropy ? g_forceAnisotropy : description.m_maxAnisotropy;
	m_sampler.ComparisonFunc = D3D12_COMPARISON_FUNC( description.m_comparisonFunc );
	m_sampler.BorderColor[0] = description.m_borderColor[0];
	m_sampler.BorderColor[1] = description.m_borderColor[1];
	m_sampler.BorderColor[2] = description.m_borderColor[2];
	m_sampler.BorderColor[3] = description.m_borderColor[3];
	m_sampler.MinLOD = description.m_minLOD;
	m_sampler.MaxLOD = description.m_mipFilter == TF_NONE ? description.m_minLOD : description.m_maxLOD;

	m_isValid = true;

	m_samplerState = nullptr;
	FORWARD_HR( renderContext.CreateSamplerState( m_sampler, m_samplerState ) );
	m_indexInHeap = m_samplerState->GetIndexInHeap();
	return S_OK;
}

void Tr2SamplerStateAL::Destroy()
{
	m_samplerState = nullptr;
	m_isValid = false;
	m_indexInHeap = 0xffffffff;
}

uint32_t Tr2SamplerStateAL::GetIndexInHeap() const
{
	return m_indexInHeap;
}

bool Tr2SamplerStateAL::IsValid() const
{
	return m_isValid;
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
	return S_OK;
}

}

#endif