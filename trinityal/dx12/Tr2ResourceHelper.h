#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../ALResult.h"
#include "../Tr2RenderContextEnum.h"

class Tr2PrimaryRenderContextAL;
class Tr2RenderContextAL;

namespace TrinityALImpl
{
	class Tr2ResourceHelper
	{
	public:
		enum Strategy
		{
			STATIC,
			SEMI_DYNAMIC,
			DYNAMIC,
		};

		Tr2ResourceHelper();

		ALResult Create( Strategy strategy, size_t size, D3D12_RESOURCE_FLAGS resourceFlags, D3D12_RESOURCE_STATES state, bool requiresImmediateBarriers, uint32_t initialDataCount, const D3D12_SUBRESOURCE_DATA* initialData, Tr2PrimaryRenderContextAL& renderContext );
		void Destroy( Tr2PrimaryRenderContextAL& renderContext );

		ALResult MapForWriting( void*& data, Tr2LockType::Type lockType, Tr2PrimaryRenderContextAL& renderContext );
		void UnmapForWriting( Tr2PrimaryRenderContextAL& renderContext );

		ALResult UpdateBuffer( Tr2LockType::Type lockType, uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL& renderContext );

		ID3D12Resource* GetResource() const;
		D3D12_GPU_VIRTUAL_ADDRESS GetGpuView() const;

		void SetName( const char* name );
		const std::string& GetName() const;
	private:
		struct Resource
		{
			CComPtr<ID3D12Resource> resource;
			uint64_t frameIndex;
			void* cpuAddress;
			D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
		};

		ALResult CreateScratch( Resource& resource, Tr2PrimaryRenderContextAL& renderContext )
		{
			return CreateScratch( resource, renderContext, m_size );
		}
		ALResult CreateScratch( Resource& resource, Tr2PrimaryRenderContextAL& renderContext, size_t size );

		std::vector<Resource> m_resources;
		Resource m_gpuResource;
		Resource m_mapped;
		size_t m_size;
		D3D12_RESOURCE_STATES m_defaultState;
		Strategy m_strategy;
		std::string m_name;
		bool m_requiresImmediateBarriers;
	};
	}

#endif