#pragma once


#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#include "../include/Tr2SamplerStateAL.h"

namespace TrinityALImpl
{
	class Tr2SamplerStateAL : public Tr2DeviceResourceAL<Tr2SamplerStateAL>
	{
	public:
		Tr2SamplerStateAL();

		ALResult Create( const Tr2SamplerDescription& description, Tr2RenderContextAL& renderContext );
		void Destroy();

		bool IsValid() const;

		Tr2ALMemoryType GetMemoryClass() const;
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
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
