#pragma once

#if( TRINITY_PLATFORM == TRINITY_METAL )

#include "../include/Tr2ConstantBufferAL.h"
#include "MetalContext.h"

namespace TrinityALImpl
{
	class Tr2ConstantBufferAL : public Tr2DeviceResourceAL<Tr2ConstantBufferAL>
	{
	public:
		Tr2ConstantBufferAL();
		~Tr2ConstantBufferAL();

		ALResult Create( uint32_t size, Tr2ConstantUsageAL::Type usage, const void* initialData, Tr2RenderContextAL & renderContext );
		void Destroy();

		ALResult Lock( void** data, Tr2RenderContextAL & renderContext );
		ALResult Unlock( Tr2RenderContextAL & renderContext );

		bool IsValid() const;
		uint32_t GetSize() const;
		Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

		ALResult SetConstants( Tr2RenderContextEnum::ShaderType, uint32_t constantIndex, Tr2RenderContextAL& renderContext );
	private:
		Tr2ConstantBufferAL( const Tr2ConstantBufferAL& ) /* = delete */;
		Tr2ConstantBufferAL& operator=( const Tr2ConstantBufferAL& ) /* = delete */;

		MetalContext *m_metalContext;
		void* m_buffer;
		uint64_t m_lockTag;
		uint32_t m_size;
		std::string m_name;

		friend class Tr2RenderContextAL;
	};
}

#endif
