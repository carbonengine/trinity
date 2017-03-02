#pragma once
#ifndef Tr2SamplerStateALDx9_H
#define Tr2SamplerStateALDx9_H

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

class Tr2SamplerStateAL: public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SAMPLER_STATE>
{
public:
	Tr2SamplerStateAL();

	ALResult Create( 
		Tr2RenderContextAL& renderContext,
		const Tr2SamplerDescription& description );
	bool IsValid() const;

	bool operator==( const Tr2SamplerStateAL& ) const;

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }

	static const uint32_t SAMPLER_STATE_MIN = 1;
	static const uint32_t SAMPLER_STATE_COUNT = 11;

	uint32_t m_states[SAMPLER_STATE_COUNT];
private:
	Tr2SamplerStateAL( const Tr2SamplerStateAL& ) /* = delete */;
	Tr2SamplerStateAL& operator=( const Tr2SamplerStateAL& ) /* = delete */;

	bool m_isValid;
};

#endif // TRINITY_PLATFORM==TRINITY_DIRECTX9

#endif // Tr2SamplerStateALDx9_H
