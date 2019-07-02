////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../Tr2TrackedALObject.h"
#include "../ALResult.h"

#include "./util/DescriptorHeapViewDx12.h"

class Tr2PrimaryRenderContextAL;
struct Tr2SamplerDescription;


namespace TrinityALImpl
{
	class Tr2SamplerStateAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SAMPLER_STATE>
	{
	public:
		Tr2SamplerStateAL();

		ALResult Create( const Tr2SamplerDescription& description, Tr2PrimaryRenderContextAL &renderContext );

		void Destroy();

		bool IsValid() const;
		Tr2ALMemoryType GetMemoryClass() const;
	private:
		std::shared_ptr<SamplerStateDx12> m_samplerState;
		D3D12_SAMPLER_DESC m_sampler;
		bool m_isValid;

		friend class Tr2RenderContextAL;
		friend class Tr2ResourceSetAL;
	};

}

#endif
