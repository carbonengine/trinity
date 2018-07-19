#pragma once
#ifndef Tr2GpuTimerALDx9_H
#define Tr2GpuTimerALDx9_H

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"


class Tr2PrimaryRenderContextAL;
class Tr2RenderContext;

namespace TrinityALImpl
{
	class Tr2GpuTimerAL :
		public Tr2TrackedALObject<Tr2RenderContextEnum::OT_TIMER>
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
	private:
		CComPtr<IDirect3DQuery9> m_beginQuery;
		CComPtr<IDirect3DQuery9> m_endQuery;
		CComPtr<IDirect3DQuery9> m_disjointQuery;
		CComPtr<IDirect3DQuery9> m_frequencyQuery;
		uint64_t m_begin;
		uint64_t m_end;
		float m_lastTime;
		enum
		{
			UNINITIALIZED,
			READY,
			BEGIN_ISSUED,
			END_ISSUED,
			BEGIN_RECEIVED,
		} m_state;
	};
}

#endif

#endif
