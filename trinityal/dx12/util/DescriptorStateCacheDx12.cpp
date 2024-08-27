////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "DescriptorStateCacheDx12.h"

#include "../Tr2PrimaryRenderContextDx12.h"

/** */
DescriptorStateCache::DescriptorStateCache( CComPtr<ID3D12Device> device, Tr2PrimaryRenderContextAL* context, uint32_t pageSizeSampler ) :
	m_allocatorSampler(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, pageSizeSampler),
	m_primaryContext(context),
	m_device(device),
	m_uploadedSamplerCount( 0 )
{
	m_allocatorUpload.Initialize( device );

	D3D12_SHADER_RESOURCE_VIEW_DESC nullSrvDesc = {};
	nullSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	nullSrvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	nullSrvDesc.Texture2D.MipLevels = 1;
	context->CreateShaderResourceView(nullptr, nullSrvDesc, m_nullSrv);

	D3D12_UNORDERED_ACCESS_VIEW_DESC nullUavDesc = {};
	nullUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	nullUavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	context->CreateUnorderedAccessView(nullptr, nullptr, nullUavDesc, m_nullUav);

	D3D12_SAMPLER_DESC nullSamplerDesc = { D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0, 0, D3D12_COMPARISON_FUNC_ALWAYS };
	nullSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	nullSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	nullSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	nullSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	nullSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	context->CreateSamplerState(nullSamplerDesc, m_nullSampler);

	Reset();
}

/** Dirty all states and reset internal allocators */
void DescriptorStateCache::Reset()
{
	for (uint32_t slot = 0; slot < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++slot)
	{
		m_srvUav[slot] = m_nullSrv;
		m_sampler[slot] = m_nullSampler;

		m_parameterSlots[Tr2RenderContextEnum::GRAPHICS_PIPE][slot].SetNone();
		m_parameterSlots[Tr2RenderContextEnum::COMPUTE_PIPE][slot].SetNone();
	}
	memset(m_cbv, 0, sizeof(m_cbv));

	m_heapsDirty = true;
	m_srvUavDirty = true;
	m_samplerDirty = true;

	m_uploadedSamplerCount = 0;

	m_pipeDirty[Tr2RenderContextEnum::GRAPHICS_PIPE] = true;
	m_pipeDirty[Tr2RenderContextEnum::COMPUTE_PIPE] = true;

	m_allocatorSampler.Reset();

	std::vector<CComPtr<ID3D12Resource>> releaseLater;
	m_allocatorUpload.Reset( releaseLater );
	for( auto& it : releaseLater )
	{
		m_primaryContext->ReleaseLater( it );
	}

	m_rootSignature = nullptr;
}

/** Dirty heap cache, this will force a call to SetDescriptorHeaps on the next Commit() */
void DescriptorStateCache::Dirty()
{
	m_heapsDirty = true;
	m_srvUavDirty = true;
	m_samplerDirty = true;

	// Set the last slots to 0 so if we need any then we will resend the descriptors to the gpu
	m_uploadedSamplerCount = 0;

	// Pretend that nothing is currently bound forcing the next Commit() to re-assign every parameter
	for (uint32_t slot = 0; slot < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++slot)
	{
		m_parameterSlots[Tr2RenderContextEnum::GRAPHICS_PIPE][slot].SetNone();
		m_parameterSlots[Tr2RenderContextEnum::COMPUTE_PIPE][slot].SetNone();
	}
}

/** Set an array of ShaderResourceViews */
void DescriptorStateCache::SetShaderResources(uint32_t startSlot, uint32_t numViews, std::shared_ptr<ShaderResourceViewDx12>* shaderResourceViews)
{
	for (uint32_t slot = 0; slot < numViews; ++slot)
	{
		uint32_t writeSlot = startSlot + slot;
		auto& srv = shaderResourceViews[slot] != nullptr ? shaderResourceViews[slot] : m_nullSrv;
		if( m_srvUav[writeSlot] != srv )
		{
			m_srvUav[writeSlot] = srv;
			m_srvUavDirty = true;
		}
	}
}

/** Set an array or UnorderedAccessViews */
void DescriptorStateCache::SetUnorderedAccessViews(uint32_t startSlot, uint32_t numViews, std::shared_ptr<UnorderedAccessViewDx12>* unorderedAccessViews)
{
	for (uint32_t slot = 0; slot < numViews; ++slot)
	{
		uint32_t writeSlot = startSlot + slot;

		auto& uav = unorderedAccessViews[slot] != nullptr ? unorderedAccessViews[slot] : m_nullUav;
		if( m_srvUav[writeSlot] != uav )
		{
			m_srvUav[writeSlot] = uav;
			m_srvUavDirty = true;
		}
	}
}

/** Set an array of SamplerStates */
void DescriptorStateCache::SetSamplers(uint32_t startSlot, uint32_t numViews, std::shared_ptr<SamplerStateDx12>* samplers)
{
	for (uint32_t slot = 0; slot < numViews; ++slot)
	{
		uint32_t writeSlot = startSlot + slot;
		auto& sampler = samplers[slot] != nullptr ? samplers[slot] : m_nullSampler;
		if( m_sampler[writeSlot] != sampler )
		{
			m_sampler[writeSlot] = sampler;
			m_samplerDirty = true;
		}
	}
}

void DescriptorStateCache::SetConstantBuffers( Tr2RenderContextEnum::ShaderType shaderStage, uint32_t slot, const TrinityALImpl::Tr2ConstantBufferAL& constantBuffer )
{
	m_cbv[shaderStage][slot] = UploadConstants( constantBuffer );
}

D3D12_GPU_VIRTUAL_ADDRESS DescriptorStateCache::UploadConstants( const TrinityALImpl::Tr2ConstantBufferAL& constantBuffer )
{
	uint64_t frameNr = m_primaryContext->GetRenderedFrameNumber();
	D3D12_GPU_VIRTUAL_ADDRESS addr = 0;

	// CB isn't resident
	if( constantBuffer.m_token.m_frameNumber != frameNr )
	{
		auto entry = m_allocatorUpload.Allocate( constantBuffer.GetDataPtr(), constantBuffer.GetSize() );
		if( entry.m_cpuAddr != nullptr )
		{
			addr = entry.m_gpuAddr;
		}

		constantBuffer.m_token.m_address = addr;
		constantBuffer.m_token.m_frameNumber = frameNr;
	}
	else
	{
		addr = constantBuffer.m_token.m_address;
	}
	return addr;
}

D3D12_GPU_VIRTUAL_ADDRESS DescriptorStateCache::UploadConstants( const void* data, uint32_t size )
{
	D3D12_GPU_VIRTUAL_ADDRESS addr = 0;
	auto entry = m_allocatorUpload.Allocate( data, size );
	if( entry.m_cpuAddr != nullptr )
	{
		addr = entry.m_gpuAddr;
	}
	return addr;
}


void DescriptorStateCache::SetHeaps( ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* globalSrvUavHeap )
{
	if( m_heapsDirty )
	{
		ID3D12DescriptorHeap* heaps[] = {
			globalSrvUavHeap,
			m_allocatorSampler.GetCurrentHeap()
		};

		commandList->SetDescriptorHeaps( 2, heaps );
	}
	m_pipeDirty[Tr2RenderContextEnum::GRAPHICS_PIPE] = true;
	m_pipeDirty[Tr2RenderContextEnum::COMPUTE_PIPE] = true;
	m_heapsDirty = false;
	m_srvUavDirty = true;
	m_samplerDirty = true;
}

/** Apply state */
void DescriptorStateCache::Commit( ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* globalSrvUavHeap, const TrinityALImpl::Tr2RootSignatureAL* rootSignature )
{
	Tr2RenderContextEnum::ShaderPipe targetPipe = rootSignature->m_isCompute ? Tr2RenderContextEnum::COMPUTE_PIPE : Tr2RenderContextEnum::GRAPHICS_PIPE;
	Tr2RenderContextEnum::ShaderPipe otherPipe = rootSignature->m_isCompute ? Tr2RenderContextEnum::GRAPHICS_PIPE : Tr2RenderContextEnum::COMPUTE_PIPE;

	bool mustBindSrvUav = m_pipeDirty[targetPipe] || m_srvUavDirty || globalSrvUavHeap != globalSrvUavHeap;
	bool mustBindSampler = m_pipeDirty[targetPipe] || m_samplerDirty || m_uploadedSamplerCount < rootSignature->m_samplerParameterCount;

	bool dirtyHeaps = m_heapsDirty;

	if( mustBindSampler )
	{
		uint32_t requiredViews = rootSignature->m_samplerParameterCount;

		// Allocate space in the heap
		DescriptorHeapEntry result = m_allocatorSampler.Allocate(requiredViews);
		dirtyHeaps |= result.m_isDirty;
		m_lastSamplerAddress[targetPipe] = result.m_gpuHandle;

		// Iterate over bindings and copy to GPU-visible heap
		uint32_t destIncrement = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		for (uint32_t idx = 0; idx < requiredViews; ++idx)
		{
			CCP_ASSERT(m_sampler[idx] != nullptr);

			D3D12_CPU_DESCRIPTOR_HANDLE dest = { result.m_cpuHandle.ptr + destIncrement * idx };
			m_device->CopyDescriptorsSimple(1, dest, m_sampler[idx]->GetHandleCPU(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		}
		m_uploadedSamplerCount = rootSignature->m_samplerParameterCount;
	}

	bool forceSet = rootSignature->m_rootSignature != m_rootSignature || m_globalSrvUavHeap != globalSrvUavHeap;

	if( dirtyHeaps || m_globalSrvUavHeap != globalSrvUavHeap )
	{
		ID3D12DescriptorHeap* heaps[] =
		{
			globalSrvUavHeap,
			m_allocatorSampler.GetCurrentHeap()
		};
		m_globalSrvUavHeap = globalSrvUavHeap;

		commandList->SetDescriptorHeaps(2, heaps);

		// If we're required to switch pipes in the future, we must force it to re-bind contents
		m_heapsDirty = false;
	}

	// Check if the SRV/UAV table *can* be bound, *must* be bound and isn't already bound
	if( mustBindSrvUav || forceSet )
	{
		auto setRootDescriptorTable = rootSignature->m_isCompute ? &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable : &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable;

		for( uint32_t i = 0; i < rootSignature->m_srvUavParameterCount; ++i )
		{
			uint32_t index = rootSignature->m_srvUavParameterOffset + i;
			auto handle = m_srvUav[index]->GetHandleGPU();
			if( forceSet || !m_parameterSlots[targetPipe][index].IsValidSRV( handle ) )
			{
				m_parameterSlots[targetPipe][index].SetSRV( handle );
				( commandList->*setRootDescriptorTable )( index, handle );
			}
		}
	}

	if( mustBindSampler || forceSet )
	{
		auto setRootDescriptorTable = rootSignature->m_isCompute ? &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable : &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable;

		auto handle = m_lastSamplerAddress[targetPipe];
		uint32_t destIncrement = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER );

		for( uint32_t i = 0; i < rootSignature->m_samplerParameterCount; ++i )
		{
			uint32_t index = rootSignature->m_samplerParameterOffset + i;
			//auto handle = m_sampler[index]->GetHandleGPU();
			if( forceSet || !m_parameterSlots[targetPipe][index].IsValidSampler( handle ) )
			{
				m_parameterSlots[targetPipe][index].SetSampler( handle );
				( commandList->*setRootDescriptorTable )( index, handle );
			}

			handle.ptr += destIncrement;
		}
	}

	D3D12_GPU_VIRTUAL_ADDRESS nullCbv = m_primaryContext->m_nullCB.GetGpuView();
	auto setConstantBufferView = rootSignature->m_isCompute ? &ID3D12GraphicsCommandList::SetComputeRootConstantBufferView : &ID3D12GraphicsCommandList::SetGraphicsRootConstantBufferView;
	if( rootSignature->m_rootSignature != m_rootSignature )
	{
		for( auto it = begin( rootSignature->m_cbRegisters ); it != end( rootSignature->m_cbRegisters ); ++it )
		{
			D3D12_GPU_VIRTUAL_ADDRESS address = m_cbv[it->stage][it->index];
			m_parameterSlots[targetPipe][it->parameter].SetCBV( address );
			( commandList->*setConstantBufferView )( it->parameter, address != 0 ? address : nullCbv );
		}
		m_rootSignature = rootSignature->m_rootSignature;
	}
	else
	{
		for( auto it = begin( rootSignature->m_cbRegisters ); it != end( rootSignature->m_cbRegisters ); ++it )
		{
			D3D12_GPU_VIRTUAL_ADDRESS address = m_cbv[it->stage][it->index];
			if( m_parameterSlots[targetPipe][it->parameter].IsValidCBV( address ) )
				continue;
			m_parameterSlots[targetPipe][it->parameter].SetCBV( address );
			( commandList->*setConstantBufferView )( it->parameter, address != 0 ? address : nullCbv );
		}
	}

	// Clear dirty flags
	m_pipeDirty[targetPipe] = false;
	m_pipeDirty[otherPipe] = true;
	m_srvUavDirty = false;
	m_samplerDirty = false;
	m_rootSignature = rootSignature->m_rootSignature;
}

FrameLocalDescriptorHeapAllocator& DescriptorStateCache::GetSamplerHeapAllocator()
{
	return m_allocatorSampler;
}

#endif
