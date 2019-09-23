////////////////////////////////////////////////////////////
//
//    Created:   April 2012
//    Copyright: CCP 2012
//

#pragma once


#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "../include/Tr2SwapChainAL.h"
#include "../include/Tr2TextureAL.h"


namespace TrinityALImpl
{

	class Tr2SwapChainAL :
		public Tr2DeviceResourceAL<Tr2SwapChainAL>
	{
	public:
		Tr2SwapChainAL();
		~Tr2SwapChainAL();

		ALResult Create( Tr2WindowHandle windowHandle, Tr2RenderContextAL& renderContext );
		void Destroy();

		bool IsValid() const;

		ALResult Present( Tr2RenderContextAL& renderContext );

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;

		Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_VIDEO; }
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;

		::Tr2TextureAL m_backBuffer;
	private:
		friend class Tr2RenderContextAL;

		Tr2SwapChainAL( const Tr2SwapChainAL& ) /* = delete */;
		Tr2SwapChainAL& operator=( const Tr2SwapChainAL& ) /* = delete */;

		ALResult CreateFramebuffer( Tr2RenderContextAL& renderContext );

		Tr2WindowHandle m_hWnd;
#ifdef _WIN32
		HDC m_hDC;
#endif
		uint32_t m_width;
		uint32_t m_height;
	};
}

#endif