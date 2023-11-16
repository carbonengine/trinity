#pragma once

#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "../include/Tr2GpuTimerAL.h"


namespace TrinityALImpl
{
	class Tr2GpuTimerAL :
		public Tr2DeviceResourceAL<Tr2GpuTimerAL>
	{
	public:
		Tr2GpuTimerAL();

		ALResult Create(Tr2PrimaryRenderContextAL& renderContext);
		void Destroy();

		bool Begin(Tr2RenderContextAL& renderContext);
		void End(Tr2RenderContextAL& renderContext);

		float GetTime(Tr2RenderContextAL& renderContext);

		bool IsValid() const;

		bool operator==(const Tr2GpuTimerAL& other) const
		{
			return this == &other;
		}

		Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_VIDEO; }
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

	private:
		bool m_isValid;
	};
}

#endif
