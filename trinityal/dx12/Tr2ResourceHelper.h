// Copyright © 2023 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../ALResult.h"
#include "../Tr2RenderContextEnum.h"

class Tr2PrimaryRenderContextAL;
class Tr2RenderContextAL;

namespace TrinityALImpl
{

// --------------------------------------------------------------------------------------
// Description:
//   Where asynchronous resource creation records uploads of initial data. Uploads are
//   recorded on a worker thread into a command list separate from the main one; the list
//   executes on a copy queue when available, otherwise on the direct queue.
// --------------------------------------------------------------------------------------
struct Tr2AsyncUploadDx12
{
	ID3D12GraphicsCommandList* commandList = nullptr;
	// When the command list executes on a copy queue it can't transition resources to non-copy
	// states; resources it writes decay to COMMON state once the upload completes.
	bool isCopyQueue = false;
	// Upload heaps that need to stay alive until the upload completes
	std::vector<CComPtr<ID3D12Resource>> scratches;
};

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

	// When upload is not null, the call is part of asynchronous creation: it may run on a worker thread
	// and records initial data uploads into upload->commandList instead of the main command list.
	ALResult Create( Strategy strategy, size_t size, D3D12_RESOURCE_FLAGS resourceFlags, D3D12_RESOURCE_STATES state, bool requiresImmediateBarriers, uint32_t initialDataCount, const D3D12_SUBRESOURCE_DATA* initialData, Tr2PrimaryRenderContextAL& renderContext, Tr2AsyncUploadDx12* upload = nullptr );
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