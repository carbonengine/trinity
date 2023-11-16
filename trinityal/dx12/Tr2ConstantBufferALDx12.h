////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../include/Tr2ConstantBufferAL.h"
#include "Tr2ResourceHelper.h"

namespace TrinityALImpl
{

	class Tr2ConstantBufferAL : public Tr2DeviceResourceAL<Tr2ConstantBufferAL>
	{
	public:

		/** JB: A token which stored a previously allocated frame-local instance of this buffer */
		struct GPUViewToken
		{
			GPUViewToken() : m_address( 0 ), m_frameNumber( UINT64_MAX ) { }

			D3D12_GPU_VIRTUAL_ADDRESS m_address;
			uint64_t m_frameNumber;
		};

		Tr2ConstantBufferAL();
		~Tr2ConstantBufferAL();

		ALResult Create( uint32_t size, Tr2ConstantUsageAL::Type usage, const void* initialData, Tr2PrimaryRenderContextAL & renderContext );
		void Destroy();

		ALResult Lock( void** data, Tr2RenderContextAL & renderContext );
		ALResult Unlock( Tr2RenderContextAL & renderContext );

		bool IsValid() const;

		uint32_t GetSize() const;
		Tr2ALMemoryType GetMemoryClass() const;

		void* GetDataPtr() const { return (void*)m_data.get(); }
		const GPUViewToken& GetToken() const { return m_token; }
		void SetToken( const GPUViewToken& token ) { m_token = token; }
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

	private:
		Tr2ConstantBufferAL( const Tr2ConstantBufferAL& ) /* = delete */;
		Tr2ConstantBufferAL& operator=( const Tr2ConstantBufferAL& ) /* = delete */;

		Tr2PrimaryRenderContextAL* m_owner;
		GPUViewToken m_token;

		CcpMallocBuffer m_data;
		uint32_t m_size;

		std::string m_name;

		friend class Tr2RenderContextAL;
		friend class DescriptorStateCache;
	};
}

#endif
