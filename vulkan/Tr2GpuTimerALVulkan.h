////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_VULKAN


#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"


class Tr2PrimaryRenderContextAL;
class Tr2RenderContextAL;


namespace TrinityALImpl
{
	class Tr2GpuTimerAL :
		public Tr2TrackedALObject<Tr2RenderContextEnum::OT_TIMER>
	{
	public:
		Tr2GpuTimerAL()
		{

		}

		ALResult Create( Tr2PrimaryRenderContextAL& renderContext )
		{
			return E_NOTIMPL;
		}

		void Destroy()
		{

		}

		bool Begin( Tr2RenderContextAL& renderContext )
		{
			return false;
		}

		void End( Tr2RenderContextAL& renderContext )
		{

		}

		float GetTime( Tr2RenderContextAL& renderContext )
		{
			return 0;
		}

		bool IsValid() const
		{
			return false;
		}

		bool operator==(const Tr2GpuTimerAL& other) const
		{
			return this == &other;
		}

		Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_VIDEO; }
	};
}


#endif
