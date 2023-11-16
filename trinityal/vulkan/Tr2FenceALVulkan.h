////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "../include/Tr2FenceAL.h"

namespace TrinityALImpl
{
	class Tr2FenceAL :
		public Tr2DeviceResourceAL<Tr2FenceAL>
	{
	public:
		Tr2FenceAL()
		{

		}
		~Tr2FenceAL()
		{

		}

		ALResult Create( Tr2PrimaryRenderContextAL& renderContext )
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

		ALResult PutFence( Tr2RenderContextAL& renderContext )
		{
			return E_NOTIMPL;
		}
		ALResult IsReached( bool& isReached, Tr2RenderContextAL& renderContext )
		{
			return E_NOTIMPL;
		}
		ALResult Wait( Tr2RenderContextAL& renderContext )
		{
			return E_NOTIMPL;
		}

		bool operator==( const Tr2FenceAL& other ) const
		{
			return this == &other;
		}

		Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_VIDEO; }
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const
		{
		}
		ALResult SetName( const char* name )
		{
			return E_NOTIMPL;
		}

	private:
		Tr2FenceAL( const Tr2FenceAL& ) /* = delete */;
		Tr2FenceAL& operator=( const Tr2FenceAL& ) /* = delete */;
	};
}

#endif
