#pragma once

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#include "../include/Tr2OcclusionQueryAL.h"

namespace TrinityALImpl
{
	class Tr2OcclusionQueryAL :
		public Tr2DeviceResourceAL<Tr2OcclusionQueryAL>
	{
	public:
		Tr2OcclusionQueryAL();
		~Tr2OcclusionQueryAL();

		ALResult Create( Tr2RenderContextAL& renderContext );
		bool IsValid() const;
		void Destroy();

		ALResult Begin( Tr2RenderContextAL& renderContext );
		ALResult End( Tr2RenderContextAL& renderContext );
		ALResult GetPixelCount( Tr2RenderContextAL& renderContext, uint32_t& count, ::Tr2OcclusionQueryAL::WaitMode waitMode );

		bool operator==( const Tr2OcclusionQueryAL& other ) const { return m_query == other.m_query; }

		Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_VIDEO; }
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	private:
		Tr2OcclusionQueryAL( const Tr2OcclusionQueryAL& ) /* = delete */;
		Tr2OcclusionQueryAL& operator=( const Tr2OcclusionQueryAL& ) /* = delete */;

		CComPtr<IDirect3DQuery9> m_query;
	};
}

#endif
