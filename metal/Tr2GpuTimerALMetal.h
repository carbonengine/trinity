#pragma once

#if( TRINITY_PLATFORM == TRINITY_METAL )

#include "../include/Tr2GpuTimerAL.h"
#include "MetalContext.h"


namespace TrinityALImpl
{
	class Tr2GpuTimerAL : public Tr2DeviceResourceAL<Tr2GpuTimerAL>
	{
	public:
		Tr2GpuTimerAL();
		~Tr2GpuTimerAL();

		ALResult Create( Tr2PrimaryRenderContextAL& renderContext );
		void Destroy();

		bool Begin( Tr2RenderContextAL& renderContext );
		void End( Tr2RenderContextAL& renderContext );

		float GetTime( Tr2RenderContextAL& renderContext );

		bool IsValid() const;

		bool operator==( const Tr2GpuTimerAL& other ) const
		{
			return this == &other;
		}

		Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_VIDEO; }
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;

	private:
		id m_buffer;
		float m_lastTime;
		enum
		{
			READY,
			BEGIN_ISSUED,
			END_ISSUED,
		} m_state;
	};
}

#endif
