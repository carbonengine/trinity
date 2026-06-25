// Copyright © 2023 CCP ehf.

#pragma once

#if ( TRINITY_PLATFORM == TRINITY_DIRECTX11 )

#include "../include/Tr2SamplerStateAL.h"


namespace TrinityALImpl
{
class Tr2SamplerStateAL : public Tr2DeviceResourceAL<Tr2SamplerStateAL>
{
public:
	Tr2SamplerStateAL();

	ALResult Create( const Tr2SamplerDescription& description, Tr2PrimaryRenderContextAL& renderContext );
	void Destroy();

	uint32_t GetIndexInHeap() const;

	bool IsValid() const;

	Tr2ALMemoryType GetMemoryClass() const;
	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	ALResult SetName( const char* name );

private:
	CComPtr<ID3D11SamplerState> m_samplerState;
	std::string m_name;
	friend class Tr2RenderContextAL;
	friend class Tr2ResourceSetAL;
};

}

#endif
