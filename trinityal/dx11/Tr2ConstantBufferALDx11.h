#pragma once


#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "../include/Tr2ConstantBufferAL.h"

#ifdef TRINITY_AL_GUARD_LOCKS
#include "../Tr2LockGuard.h"
#endif

namespace TrinityALImpl
{
	class Tr2ConstantBufferAL : public Tr2DeviceResourceAL<Tr2ConstantBufferAL>
	{
	public:
		Tr2ConstantBufferAL();

		ALResult Create( uint32_t size, Tr2ConstantUsageAL::Type usage, const void* initialData, Tr2PrimaryRenderContextAL & renderContext );
		void Destroy();

		ALResult Lock( void** data, Tr2RenderContextAL & renderContext );
		ALResult Unlock( Tr2RenderContextAL & renderContext );

		bool IsValid() const;
		Tr2ConstantUsageAL::Type GetUsage() const;
		uint32_t GetSize() const;
		Tr2ALMemoryType GetMemoryClass() const;

		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

	private:
		Tr2ConstantBufferAL( const Tr2ConstantBufferAL& ) /* = delete */;
		Tr2ConstantBufferAL& operator=( const Tr2ConstantBufferAL& ) /* = delete */;

		CComPtr<ID3D11Buffer>	m_buffer;
		CcpMallocBuffer	m_bufferMirror;
		Tr2ConstantUsageAL::Type m_usage;
		uint32_t	m_size;

#ifdef TRINITY_AL_GUARD_LOCKS
		Tr2LockGuard m_lockGuard;
#endif
		std::string m_name;

		friend class Tr2RenderContextAL;
	};

}
#endif 
