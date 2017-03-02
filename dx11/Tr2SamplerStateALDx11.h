#pragma once
#ifndef Tr2SamplerStateALDx11_H
#define Tr2SamplerStateALDx11_H

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

// -------------------------------------------------------------
// Description:
//	Setting shader sampler state in a platform neutral way.
// -------------------------------------------------------------
class Tr2SamplerStateAL: public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SAMPLER_STATE>
{
public:
	Tr2SamplerStateAL();
	ALResult Create( 
		Tr2PrimaryRenderContextAL &renderContext, 
		const Tr2SamplerDescription& description );

	bool IsValid() const 
	{ 
		return m_samplerState != nullptr; 
	}

	bool operator==( const Tr2SamplerStateAL& ) const;

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }

	CComPtr<ID3D11SamplerState> m_samplerState;
	D3D11_SAMPLER_DESC m_samplerDesc;
};

#endif // TRINITY_PLATFORM==TRINITY_DIRECTX11

#endif // Tr2SamplerStateALDx11_H