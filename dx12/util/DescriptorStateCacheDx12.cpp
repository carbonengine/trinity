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
DescriptorStateCache::DescriptorStateCache(CComPtr<ID3D12Device> device, Tr2PrimaryRenderContextAL* context, uint32_t pageSizeCBVSRVUAV, uint32_t pageSizeSampler) :
	m_device(device),
	m_allocatorSRV(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, pageSizeCBVSRVUAV),
	m_allocatorSampler(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, pageSizeSampler)
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
	}
	memset(m_cbv, 0, sizeof(m_cbv));
	memset(m_cbvDirty, 1, sizeof(m_cbvDirty));

	m_heapsDirty = true;
	m_srvUavDirty = true;
	m_samplerDirty = true;

	m_pipeDirty[Tr2RenderContextEnum::GRAPHICS_PIPE] = true;
	m_pipeDirty[Tr2RenderContextEnum::COMPUTE_PIPE] = true;

	m_allocatorSRV.Reset();
	m_allocatorSampler.Reset();
}

/** Dirty heap cache, this will force a call to SetDescriptorHeaps on the next Commit() */
void DescriptorStateCache::Dirty()
{
	m_heapsDirty = true;
	memset(m_cbvDirty, 1, sizeof(m_cbvDirty));
}

/** Set an array of ShaderResourceViews */
void DescriptorStateCache::SetShaderResources(uint32_t startSlot, uint32_t numViews, std::shared_ptr<ShaderResourceViewDx12>* shaderResourceViews)
{
	for (uint32_t slot = 0; slot < numViews; ++slot)
	{
		uint32_t writeSlot = startSlot + slot;

		if ((m_srvUav[writeSlot] == m_nullSrv ? nullptr : m_srvUav[writeSlot]) != shaderResourceViews[slot])
		{
			m_srvUav[writeSlot] = shaderResourceViews[slot] != nullptr ? shaderResourceViews[slot] : m_nullSrv;
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

		if ((m_srvUav[writeSlot] == m_nullUav ? nullptr : m_srvUav[writeSlot]) != unorderedAccessViews[slot])
		{
			m_srvUav[writeSlot] = unorderedAccessViews[slot] != nullptr ? unorderedAccessViews[slot] : m_nullUav;
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

		if ((m_sampler[writeSlot] == m_nullSampler ? nullptr : m_sampler[writeSlot]) != samplers[slot])
		{
			m_sampler[writeSlot] = samplers[slot] != nullptr ? samplers[slot] : m_nullSampler;
			m_samplerDirty = true;
		}
	}
}

/** Set a constantbuffer */
void DescriptorStateCache::SetConstantBuffers(Tr2RenderContextEnum::ShaderType shaderStage, uint32_t slot, const Tr2ConstantBufferAL& constantBuffer)
{
	D3D12_GPU_VIRTUAL_ADDRESS addr = constantBuffer.m_buffer.GetGpuView();
	if(m_cbv[shaderStage][slot] == addr)
		return;

	m_cbv[shaderStage][slot] = addr;
	m_cbvDirty[shaderStage][slot] = true;
}

/** Apply state */
void DescriptorStateCache::Commit(Tr2PrimaryRenderContextAL* primaryContext, CComPtr<ID3D12GraphicsCommandList> commandList, const Tr2ShaderProgramAL* shader)
{
	Tr2RenderContextEnum::ShaderPipe targetPipe = shader->IsComputeProgramDx12() ? Tr2RenderContextEnum::COMPUTE_PIPE : Tr2RenderContextEnum::GRAPHICS_PIPE;
	Tr2RenderContextEnum::ShaderPipe otherPipe = shader->IsComputeProgramDx12() ? Tr2RenderContextEnum::GRAPHICS_PIPE : Tr2RenderContextEnum::COMPUTE_PIPE;

	bool mustBindSrvUav = m_pipeDirty[targetPipe] || m_srvUavDirty;
	bool mustBindSampler = m_pipeDirty[targetPipe] || m_samplerDirty;

	bool dirtyHeaps = m_heapsDirty;

	if (mustBindSrvUav)
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
	}

	if (mustBindSampler)
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
		m_pipeDirty[otherPipe] = true;
		m_heapsDirty = false;
	}

	if ((mustBindSrvUav || m_lastSrvUavParameter[targetPipe] != shader->m_srvUavParameter) && shader->m_srvUavParameter != 0xffffffff)
	{
		if (shader->IsComputeProgramDx12())
		{
			commandList->SetComputeRootDescriptorTable(shader->m_srvUavParameter, m_lastSrvUavAddress[targetPipe]);
		}
		else
		{
			commandList->SetGraphicsRootDescriptorTable(shader->m_srvUavParameter, m_lastSrvUavAddress[targetPipe]);
		}
	}

	if ((mustBindSampler || m_lastSamplerParameter[targetPipe] != shader->m_samplerParameter) && shader->m_samplerParameter != 0xffffffff)
	{
		if (shader->IsComputeProgramDx12())
		{
			commandList->SetComputeRootDescriptorTable(shader->m_samplerParameter, m_lastSamplerAddress[targetPipe]);
		}
		else
		{
			commandList->SetGraphicsRootDescriptorTable(shader->m_samplerParameter, m_lastSamplerAddress[targetPipe]);
		}
	}

	if (shader->IsComputeProgramDx12())
	{
		for (auto it = begin(shader->m_cbRegisters); it != end(shader->m_cbRegisters); ++it)
		{
			if (!m_cbvDirty[it->stage][it->index])
				continue;
			m_cbvDirty[it->stage][it->index] = false;

			if (m_cbv[it->stage][it->index])
			{
				commandList->SetComputeRootConstantBufferView(it->parameter, m_cbv[it->stage][it->index]);
			}
			else
			{
				commandList->SetComputeRootConstantBufferView(it->parameter, primaryContext->m_nullCB.GetGpuView());
			}
		}
	}
	else
	{
		for (auto it = begin(shader->m_cbRegisters); it != end(shader->m_cbRegisters); ++it)
		{
			if (!m_cbvDirty[it->stage][it->index])
				continue;
			m_cbvDirty[it->stage][it->index] = false;

			if (m_cbv[it->stage][it->index])
			{
				commandList->SetGraphicsRootConstantBufferView(it->parameter, m_cbv[it->stage][it->index]);
			}
			else
			{
				commandList->SetGraphicsRootConstantBufferView(it->parameter, primaryContext->m_nullCB.GetGpuView());
			}
		}
	}

	// Clear dirty flags
	m_pipeDirty[targetPipe] = false;
	m_srvUavDirty = false;
	m_samplerDirty = false;
}

#endif
