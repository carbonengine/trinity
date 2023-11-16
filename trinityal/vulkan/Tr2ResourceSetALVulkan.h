////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "../include/Tr2ResourceSetAL.h"

namespace TrinityALImpl
{
	class Tr2ResourceSetAL: public Tr2DeviceResourceAL<Tr2ResourceSetAL>
	{
	public:
		Tr2ResourceSetAL();
		~Tr2ResourceSetAL();

		ALResult Create( const Tr2ResourceSetDescriptionAL& description, const ::Tr2ShaderProgramAL& program, Tr2PrimaryRenderContextAL& renderContext );
		void Destroy();

		bool IsValid() const;
		Tr2ALMemoryType GetMemoryClass() const;
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

	private:
		Tr2PrimaryRenderContextAL *m_owner;

		VkDescriptorPool m_pool;
		VkDescriptorSet m_descriptorSet;

		friend class ::Tr2RenderContextAL;
	};
}

#endif