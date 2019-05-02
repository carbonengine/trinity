////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "../Tr2TrackedALObject.h"
#include "../ALResult.h"


class Tr2PrimaryRenderContextAL;
struct Tr2SamplerDescription;


namespace TrinityALImpl
{
	class Tr2SamplerStateAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SAMPLER_STATE>
	{
	public:
		Tr2SamplerStateAL()
		{
		}

		ALResult Create( const Tr2SamplerDescription& description, Tr2PrimaryRenderContextAL &renderContext )
		{
			return E_NOTIMPL;
		}

		void Destroy()
		{
		}

		bool IsValid() const
		{
			return false;
		}

		Tr2ALMemoryType GetMemoryClass()
		{
			return AL_MEMORY_VIDEO;
		}
	};

}

#endif
