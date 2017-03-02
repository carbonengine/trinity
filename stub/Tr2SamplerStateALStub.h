#pragma once
#ifndef Tr2SamplerStateALStub_H
#define Tr2SamplerStateALStub_H

#if( TRINITY_PLATFORM==TRINITY_STUB )

class Tr2SamplerStateAL: public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SAMPLER_STATE>
{
public:
	Tr2SamplerStateAL();

	ALResult Create( 
		Tr2RenderContextAL& renderContext,
		const Tr2SamplerDescription& description );
	bool IsValid() const;


	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }

private:
	bool m_isValid;
};

#endif // TRINITY_PLATFORM==TRINITY_STUB

#endif // Tr2SamplerStateALStub_H
