#pragma once

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "../include/Tr2FenceAL.h"

#ifdef __ANDROID__
typedef struct __GLsync *GLsync;
#endif


namespace TrinityALImpl
{
	class Tr2FenceAL :
		public Tr2DeviceResourceAL<Tr2FenceAL>
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
		Tr2FenceAL( const Tr2FenceAL& ) /* = delete */;
		Tr2FenceAL& operator=( const Tr2FenceAL& ) /* = delete */;

		bool m_isValid;
		GLsync m_fence;
	};
}

#endif
