////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../include/Tr2SamplerStateAL.h"
#include "./util/DescriptorHeapViewDx12.h"


namespace TrinityALImpl
{
	class Tr2SamplerStateAL : public Tr2DeviceResourceAL<Tr2SamplerStateAL>
	{
	public:
		Tr2SamplerStateAL();

		ALResult Create( const Tr2SamplerDescription& description, Tr2PrimaryRenderContextAL &renderContext );

		void Destroy();

		bool IsValid() const;
		Tr2ALMemoryType GetMemoryClass() const;
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

	private:
		std::shared_ptr<SamplerStateDx12> m_samplerState;
		D3D12_SAMPLER_DESC m_sampler;
		std::string m_name;
		bool m_isValid;

		friend class Tr2RenderContextAL;
		friend class Tr2ResourceSetAL;
	};

}

#endif
