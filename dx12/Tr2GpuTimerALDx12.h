////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12


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
		Tr2GpuTimerAL();
		~Tr2GpuTimerAL();

		ALResult Create( Tr2PrimaryRenderContextAL& renderContext );
		void Destroy();
		bool IsValid() const;

		bool Begin( Tr2RenderContextAL& renderContext );
		void End( Tr2RenderContextAL& renderContext );

		float GetTime( Tr2RenderContextAL& renderContext );

		bool operator==( const Tr2GpuTimerAL& other ) const;
		Tr2ALMemoryType GetMemoryClass() const;
	private:
		CComPtr<ID3D12QueryHeap> m_query;
		CComPtr<ID3D12Resource> m_result;
		uint64_t m_frameIndex;
		Tr2PrimaryRenderContextAL* m_owner;
		float m_lastTime;
	};
}


#endif
