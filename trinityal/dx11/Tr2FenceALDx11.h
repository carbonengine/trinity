#pragma once

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "../include/Tr2FenceAL.h"

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
		ALResult SetName( const char* name );

	private:
		Tr2FenceAL( const Tr2FenceAL& ) /* = delete */;
		Tr2FenceAL& operator=( const Tr2FenceAL& ) /* = delete */;

		CComPtr<ID3D11Query> m_query;
		std::string m_name;
	};

}

#endif
