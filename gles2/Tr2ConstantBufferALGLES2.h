#pragma once

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "../include/Tr2ConstantBufferAL.h"


namespace TrinityALImpl
{
	class Tr2ConstantBufferAL : public Tr2DeviceResourceAL<Tr2ConstantBufferAL>
	{
	public:
		Tr2ConstantBufferAL();

		ALResult Create( uint32_t size, Tr2ConstantUsageAL::Type usage, const void* initialData, Tr2RenderContextAL & renderContext );
		void Destroy();

		ALResult Lock( void** data, Tr2RenderContextAL & renderContext );
		ALResult Unlock( Tr2RenderContextAL & renderContext );

		bool IsValid() const;
		uint32_t GetSize() const;
		Tr2ALMemoryType GetMemoryClass() const;
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	private:
		Tr2ConstantBufferAL( const Tr2ConstantBufferAL& ) /* = delete */;
		Tr2ConstantBufferAL& operator=( const Tr2ConstantBufferAL& ) /* = delete */;

		CcpMallocBuffer	m_shadowCopy;

		friend class ::Tr2RenderContextAL;
	};
}

#endif