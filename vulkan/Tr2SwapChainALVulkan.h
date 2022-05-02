////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "../include/Tr2SwapChainAL.h"
#include "../include/Tr2TextureAL.h"


namespace TrinityALImpl
{

	class Tr2SwapChainAL : public Tr2DeviceResourceAL<Tr2SwapChainAL>
	{
	public:
		Tr2SwapChainAL()
		{

		}

		ALResult Create( Tr2WindowHandle windowHandle, Tr2PrimaryRenderContextAL &renderContext )
		{
			return E_NOTIMPL;
		}
		void Destroy()
		{

		}

		bool IsValid() const
		{
			return false;
		}

		ALResult Present( Tr2RenderContextAL& renderContext )
		{
			return E_NOTIMPL;
		}

		uint32_t GetWidth() const
		{
			return 0;
		}
		uint32_t GetHeight() const
		{
			return 0;
		}

		::Tr2TextureAL m_backBuffer;

		bool operator==( const Tr2SwapChainAL& other ) const { return false; }

		Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const
		{
		}
		ALResult SetName( const char* name )
		{
			return E_NOTIMPL;
		}

	private:
		Tr2SwapChainAL( const Tr2SwapChainAL& ) /* = delete */;
		Tr2SwapChainAL& operator=( const Tr2SwapChainAL& ) /* = delete */;
	};
}

#endif
