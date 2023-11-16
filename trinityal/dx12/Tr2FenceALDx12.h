////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../include/Tr2FenceAL.h"

namespace TrinityALImpl
{
	class Tr2FenceAL :
		public Tr2DeviceResourceAL<Tr2FenceAL>
	{
	public:
		Tr2FenceAL();

		ALResult Create( Tr2PrimaryRenderContextAL& renderContext );
		void Destroy();
		bool IsValid() const;

		ALResult PutFence( Tr2RenderContextAL& renderContext );
		ALResult IsReached( bool& isReached, Tr2RenderContextAL& renderContext );
		ALResult Wait( Tr2RenderContextAL& renderContext );

		Tr2ALMemoryType GetMemoryClass() const;
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

	private:
		Tr2FenceAL( const Tr2FenceAL& ) /* = delete */;
		Tr2FenceAL& operator=( const Tr2FenceAL& ) /* = delete */;

		uint64_t m_frameIndex;
		Tr2PrimaryRenderContextAL* m_owner;
		std::string m_name;
	};
	}

#endif
