#pragma once
#ifndef Tr2SamplerStateALDx11_H
#define Tr2SamplerStateALDx11_H


#include "../Tr2TrackedALObject.h"
#include "../ALResult.h"


class Tr2PrimaryRenderContextAL;
struct Tr2SamplerDescription;


#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

namespace TrinityALImpl
{
	class Tr2SamplerStateAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SAMPLER_STATE>
	{
	public:
		Tr2SamplerStateAL();

		ALResult Create( const Tr2SamplerDescription& description, Tr2PrimaryRenderContextAL &renderContext );
		void Destroy();

		bool IsValid() const;

		Tr2ALMemoryType GetMemoryClass();
	private:
		CComPtr<ID3D11SamplerState> m_samplerState;
		friend class Tr2RenderContextAL;
		friend class Tr2ResourceSetAL;
	};

}

#endif

#endif