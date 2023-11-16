////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once


#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "../include/Tr2ConstantBufferAL.h"

namespace TrinityALImpl
{
	// -------------------------------------------------------------
	// Description:
	//   A low level wrapper around the calls needed to set up a constant
	//   buffer for a given platform. Any higher level logic should be
	//   handled one level up, this is only to avoid ifdef soup when
	//   creating and locking buffers.
	//	 32bit: no support for 64bit-sized buffers.
	// -------------------------------------------------------------
	class Tr2ConstantBufferAL: public Tr2DeviceResourceAL<Tr2ConstantBufferAL>
	{
	public:
		Tr2ConstantBufferAL();
		~Tr2ConstantBufferAL();

		ALResult Create( uint32_t size, Tr2ConstantUsageAL::Type usage, const void* initialData, Tr2PrimaryRenderContextAL& renderContext );

		ALResult Lock( void** data, Tr2RenderContextAL& renderContext );
		ALResult Unlock( Tr2RenderContextAL& renderContext );

		bool IsValid() const;

		void Destroy();

		Tr2ConstantUsageAL::Type GetUsage() const;
		uint32_t GetSize() const;

		Tr2ALMemoryType GetMemoryClass() const;
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

	private:
		Tr2ConstantBufferAL( const Tr2ConstantBufferAL& ) /* = delete */;
		Tr2ConstantBufferAL& operator=( const Tr2ConstantBufferAL& ) /* = delete */;

		VkBuffer m_buffer;
		VkDeviceMemory m_memory;
		Tr2PrimaryRenderContextAL* m_owner;
		uint32_t m_size;
	};
}
#endif
