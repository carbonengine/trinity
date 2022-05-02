#pragma once
#ifndef Tr2GpuTimerALDx11_H
#define Tr2GpuTimerALDx11_H

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )


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
		CComPtr<ID3D11Query> m_beginQuery;
		CComPtr<ID3D11Query> m_endQuery;
		CComPtr<ID3D11Query> m_disjointQuery;
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
		std::string m_name;


	};
}


#endif

#endif
