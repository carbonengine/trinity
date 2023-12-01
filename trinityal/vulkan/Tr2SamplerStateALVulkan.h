////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "../include/Tr2SamplerStateAL.h"

namespace TrinityALImpl
{
	class Tr2SamplerStateAL : public Tr2DeviceResourceAL<Tr2SamplerStateAL>
	{
	public:
		Tr2SamplerStateAL();
		~Tr2SamplerStateAL();

		ALResult Create( const Tr2SamplerDescription& description, Tr2PrimaryRenderContextAL &renderContext );
		void Destroy();

		bool IsValid() const;
		Tr2ALMemoryType GetMemoryClass() const;
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

	private:
		VkSampler m_sampler;
		Tr2PrimaryRenderContextAL* m_owner;

		friend class TrinityALImpl::Tr2ResourceSetAL;
	};

}

#endif
