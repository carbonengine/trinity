////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "../include/Tr2OcclusionQueryAL.h"

namespace TrinityALImpl
{
	class Tr2OcclusionQueryAL : public Tr2DeviceResourceAL<Tr2OcclusionQueryAL>
	{
	public:
		Tr2OcclusionQueryAL() {}

		ALResult Create( Tr2PrimaryRenderContextAL& renderContext )
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

		ALResult Begin( Tr2RenderContextAL& renderContext )
		{
			return E_NOTIMPL;
		}
		ALResult End( Tr2RenderContextAL& renderContext )
		{
			return E_NOTIMPL;
		}
		ALResult GetPixelCount( Tr2RenderContextAL& renderContext, uint32_t& count, ::Tr2OcclusionQueryAL::WaitMode waitMode )
		{
			return E_NOTIMPL;
		}

		bool operator==( const Tr2OcclusionQueryAL& other ) const { return false; }

		Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const
		{
		}
		ALResult SetName( const char* name )
		{
			return E_NOTIMPL;
		}

	private:
		Tr2OcclusionQueryAL( const Tr2OcclusionQueryAL& ) /* = delete */;
		Tr2OcclusionQueryAL& operator=( const Tr2OcclusionQueryAL& ) /* = delete */;
	};
}

#endif
