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
	class Tr2ResourceSetAL: public Tr2TrackedALObject<Tr2RenderContextEnum::OT_RESOURCE_SET>
	{
	public:
		Tr2ResourceSetAL()
		{

		}

		ALResult Create( const Tr2ResourceSetDescriptionAL& description, const Tr2ShaderProgramAL& program, Tr2PrimaryRenderContextAL& renderContext )
		{
			return E_NOTIMPL;
		}
		bool IsValid() const
		{
			return false;
		}

		void Destroy()
		{

		}
		Tr2ALMemoryType GetMemoryClass() const
		{
			return AL_MEMORY_VIDEO;
		}
	};
}

#endif