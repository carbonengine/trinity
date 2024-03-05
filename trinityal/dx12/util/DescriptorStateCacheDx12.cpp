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
DescriptorStateCache::DescriptorStateCache(CComPtr<ID3D12Device> device, Tr2PrimaryRenderContextAL* context, uint32_t pageSizeCBVSRVUAV, uint32_t pageSizeSampler, uint32_t pageSizeUploadDefault, uint32_t pageSizeUploadSpill) :
	m_allocatorSRV(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, pageSizeCBVSRVUAV),
	m_allocatorSampler(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, pageSizeSampler),
	m_allocatorUpload(context, pageSizeUploadDefault, pageSizeUploadSpill),
	m_primaryContext(context),
	m_device(device),
	m_uploadedSamplerCount( 0 ),
	m_uploadedSrvUavCount( 0 )
{
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
	m_uploadedSrvUavCount = 0;

	m_pipeDirty[Tr2RenderContextEnum::GRAPHICS_PIPE] = true;
	m_pipeDirty[Tr2RenderContextEnum::COMPUTE_PIPE] = true;

	m_allocatorSRV.Reset();
	m_allocatorSampler.Reset();
	m_allocatorUpload.Reset();

	m_rootSignature = nullptr;
}

/** Dirty heap cache, this will force a call to SetDescriptorHeaps on the next Commit() */
void DescriptorStateCache::Dirty()
{
	m_heapsDirty = true;

	// Set the last slots to 0 so if we need any then we will resend the descriptors to the gpu
	m_uploadedSamplerCount = 0;
	m_uploadedSrvUavCount = 0;

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

/** Set a constantbuffer */
void DescriptorStateCache::SetConstantBuffers(Tr2RenderContextEnum::ShaderType shaderStage, uint32_t slot, const TrinityALImpl::Tr2ConstantBufferAL& constantBuffer)
{
	uint64_t frameNr = m_primaryContext->GetCompletedFrameIndexDx12();
	const TrinityALImpl::Tr2ConstantBufferAL::GPUViewToken& currentToken = constantBuffer.GetToken();
	D3D12_GPU_VIRTUAL_ADDRESS addr = 0;
	
	// CB isn't resident
	if (currentToken.m_frameNumber != frameNr)
	{
		UploadHeapEntry entry = m_allocatorUpload.Allocate(constantBuffer.GetSize(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		if(entry.m_cpuAddr != nullptr)
		{
			CCP_ASSERT(entry.m_size >= constantBuffer.GetSize());

			memcpy(entry.m_cpuAddr, constantBuffer.GetDataPtr(), constantBuffer.GetSize());
			addr = entry.m_gpuAddr;
		}

		TrinityALImpl::Tr2ConstantBufferAL::GPUViewToken token;
		token.m_address = addr;
		token.m_frameNumber = frameNr;
		const_cast<TrinityALImpl::Tr2ConstantBufferAL&>(constantBuffer).SetToken(token);
	}
	else
	{
		addr = currentToken.m_address;
	}

	m_cbv[shaderStage][slot] = addr;
}

DescriptorHeapEntry DescriptorStateCache::Commit( ID3D12GraphicsCommandList* commandList, UnorderedAccessViewDx12* uav )
{
	bool dirtyHeaps = m_heapsDirty;
	DescriptorHeapEntry result = m_allocatorSRV.Allocate( 1 );
	dirtyHeaps |= result.m_isDirty;
	D3D12_CPU_DESCRIPTOR_HANDLE dest = { result.m_cpuHandle.ptr };
	m_device->CopyDescriptorsSimple( 1, dest, uav->GetHandleCPU(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

	if( dirtyHeaps )
	{
		ID3D12DescriptorHeap* heaps[] = {
			m_allocatorSRV.GetCurrentHeap(),
			m_allocatorSampler.GetCurrentHeap()
		};

		commandList->SetDescriptorHeaps( 2, heaps );

	}
	m_pipeDirty[Tr2RenderContextEnum::GRAPHICS_PIPE] = true;
	m_pipeDirty[Tr2RenderContextEnum::COMPUTE_PIPE] = true;
	m_heapsDirty = false;
	m_srvUavDirty = true;
	m_samplerDirty = true;
	return result;
}

/** Apply state */
void DescriptorStateCache::Commit( ID3D12GraphicsCommandList* commandList, const TrinityALImpl::Tr2ShaderProgramAL* shader )
{
	Tr2RenderContextEnum::ShaderPipe targetPipe = shader->IsComputeProgramDx12() ? Tr2RenderContextEnum::COMPUTE_PIPE : Tr2RenderContextEnum::GRAPHICS_PIPE;
	Tr2RenderContextEnum::ShaderPipe otherPipe = shader->IsComputeProgramDx12() ? Tr2RenderContextEnum::GRAPHICS_PIPE : Tr2RenderContextEnum::COMPUTE_PIPE;

	bool mustBindSrvUav = m_pipeDirty[targetPipe] || m_srvUavDirty || m_uploadedSrvUavCount < shader->m_srvUavTableSize;
	bool mustBindSampler = m_pipeDirty[targetPipe] || m_samplerDirty || m_uploadedSamplerCount < shader->m_samplerTableSize;

	bool dirtyHeaps = m_heapsDirty;

	if( mustBindSrvUav )
	{
		uint32_t requiredViews = shader->m_srvUavTableSize;

		// Allocate space in the heap
		DescriptorHeapEntry result = m_allocatorSRV.Allocate(requiredViews);
		dirtyHeaps |= result.m_isDirty;
		m_lastSrvUavAddress[targetPipe] = result.m_gpuHandle;

		// Iterate over bindings and copy to GPU-visible heap
		uint32_t destIncrement = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		for (uint32_t idx = 0; idx < requiredViews; ++idx)
		{
			CCP_ASSERT(m_srvUav[idx] != nullptr);

			D3D12_CPU_DESCRIPTOR_HANDLE dest = { result.m_cpuHandle.ptr + destIncrement * idx };
			m_device->CopyDescriptorsSimple(1, dest, m_srvUav[idx]->GetHandleCPU(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		m_uploadedSrvUavCount = shader->m_srvUavTableSize;
	}

	if( mustBindSampler )
	{
		uint32_t requiredViews = shader->m_samplerTableSize;

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
		m_uploadedSamplerCount = shader->m_samplerTableSize;
	}

	if (dirtyHeaps)
	{
		ID3D12DescriptorHeap* heaps[] =
		{
			m_allocatorSRV.GetCurrentHeap(),
			m_allocatorSampler.GetCurrentHeap()
		};

		commandList->SetDescriptorHeaps(2, heaps);

		// If we're required to switch pipes in the future, we must force it to re-bind contents
		m_heapsDirty = false;
	}

	// Check if the SRV/UAV table *can* be bound, *must* be bound and isn't already bound
	if( shader->m_srvUavParameter != 0xffffffff && ( mustBindSrvUav || !m_parameterSlots[targetPipe][shader->m_srvUavParameter].IsValidSRV( m_lastSrvUavAddress[targetPipe] ) || shader->m_rootSignature != m_rootSignature ) )
	{
		m_parameterSlots[targetPipe][shader->m_srvUavParameter].SetSRV(m_lastSrvUavAddress[targetPipe]);

		if (shader->IsComputeProgramDx12())
		{
			commandList->SetComputeRootDescriptorTable(shader->m_srvUavParameter, m_lastSrvUavAddress[targetPipe]);
		}
		else
		{
			commandList->SetGraphicsRootDescriptorTable(shader->m_srvUavParameter, m_lastSrvUavAddress[targetPipe]);
		}
	}

	// Check if the Sampler table *can* be bound, *must* be bound and isn't already bound
	if( shader->m_samplerParameter != 0xffffffff && ( mustBindSampler || !m_parameterSlots[targetPipe][shader->m_samplerParameter].IsValidSampler( m_lastSamplerAddress[targetPipe] ) || shader->m_rootSignature != m_rootSignature ) )
	{
		m_parameterSlots[targetPipe][shader->m_samplerParameter].SetSampler(m_lastSamplerAddress[targetPipe]);

		if (shader->IsComputeProgramDx12())
		{
			commandList->SetComputeRootDescriptorTable(shader->m_samplerParameter, m_lastSamplerAddress[targetPipe]);
		}
		else
		{
			commandList->SetGraphicsRootDescriptorTable(shader->m_samplerParameter, m_lastSamplerAddress[targetPipe]);
		} 
	}

	D3D12_GPU_VIRTUAL_ADDRESS nullCbv = m_primaryContext->m_nullCB.GetGpuView();
	auto setConstantBufferView = shader->IsComputeProgramDx12() ? &ID3D12GraphicsCommandList::SetComputeRootConstantBufferView : &ID3D12GraphicsCommandList::SetGraphicsRootConstantBufferView;
	if( shader->m_rootSignature != m_rootSignature )
	{
		for( auto it = begin( shader->m_cbRegisters ); it != end( shader->m_cbRegisters ); ++it )
		{
			D3D12_GPU_VIRTUAL_ADDRESS address = m_cbv[it->stage][it->index];
			m_parameterSlots[targetPipe][it->parameter].SetCBV( address );
			( commandList->*setConstantBufferView )( it->parameter, address != 0 ? address : nullCbv );
		}
		m_rootSignature = shader->m_rootSignature;
	}
	else
	{
		for( auto it = begin( shader->m_cbRegisters ); it != end( shader->m_cbRegisters ); ++it )
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
}

#endif
