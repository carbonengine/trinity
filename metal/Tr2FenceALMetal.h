#pragma once

#if( TRINITY_PLATFORM == TRINITY_METAL )

#include "../include/Tr2FenceAL.h"
#include "MetalContext.h"

namespace TrinityALImpl
{
	class Tr2FenceAL : public Tr2DeviceResourceAL<Tr2FenceAL>
	{
	public:
		Tr2FenceAL();
		~Tr2FenceAL();

		ALResult Create( Tr2PrimaryRenderContextAL& renderContext );
		void Destroy();

		bool IsValid() const;

		ALResult PutFence( Tr2RenderContextAL& renderContext );
		ALResult IsReached( bool& isReached, Tr2RenderContextAL& renderContext );
		ALResult Wait( Tr2RenderContextAL& renderContext );

		Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_VIDEO; }

		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;

	private:
		uint64_t m_frame;
		bool m_isValid;
		bool m_hasBeenPut;
	};
}

#endif
