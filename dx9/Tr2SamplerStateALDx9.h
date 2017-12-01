#pragma once
#ifndef Tr2SamplerStateALDx9_H
#define Tr2SamplerStateALDx9_H


#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"


class Tr2RenderContextAL;
struct Tr2SamplerDescription;


#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

namespace TrinityALImpl
{
	class Tr2SamplerStateAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SAMPLER_STATE>
	{
	public:
		Tr2SamplerStateAL();

		ALResult Create( const Tr2SamplerDescription& description, Tr2RenderContextAL& renderContext );
		bool IsValid() const;

		Tr2ALMemoryType GetMemoryClass() const;
	private:
		static const uint32_t SAMPLER_STATE_MIN = 1;
		static const uint32_t SAMPLER_STATE_COUNT = 11;

		uint32_t m_states[SAMPLER_STATE_COUNT];

		bool m_isValid;

		friend class Tr2RenderContextAL;
		friend class Tr2ResourceSetAL;
	};
}

#endif

#endif
