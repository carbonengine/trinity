////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include <memory>

#include "../Tr2ConstantBufferALDx12.h"
#include "../Tr2ShaderProgramAlDx12.h"
#include "../../Tr2RenderContextEnum.h"
#include "../../include/Tr2ResourceSetAL.h"

#include "DescriptorHeapViewDx12.h"
#include "FrameLocalDescriptorHeapAllocatorDx12.h"

/** State manager/cache for descriptor based bindings */
class DescriptorStateCache
{
public:

	/** */
	DescriptorStateCache(CComPtr<ID3D12Device> device, class Tr2PrimaryRenderContextAL* context, uint32_t pageSizeCBVSRVUAV, uint32_t pageSizeSampler);

	/** Dirty all states and reset internal allocators */
	void Reset();

	/** Dirty heap cache, this will force a call to SetDescriptorHeaps on the next Commit() */
	void Dirty();

	/** Apply state */
	void Commit(class Tr2PrimaryRenderContextAL* primaryContext, CComPtr<ID3D12GraphicsCommandList> commandList, const Tr2ShaderProgramAL* shader);

	/** Set an array of ShaderResourceViews */
	void SetShaderResources(uint32_t startSlot, uint32_t numViews, std::shared_ptr<ShaderResourceViewDx12>* shaderResourceViews);

	/** Set an array of SamplerStates */
	void SetSamplers(uint32_t startSlot, uint32_t numViews, std::shared_ptr<SamplerStateDx12>* samplers);

	/** Set a constantbuffer */
	void SetConstantBuffers(Tr2RenderContextEnum::ShaderType shaderStage, uint32_t slot, const Tr2ConstantBufferAL& constantBuffer);

	/** Set an array or UnorderedAccessViews */
	void SetUnorderedAccessViews(uint32_t startSlot, uint32_t numViews, std::shared_ptr<UnorderedAccessViewDx12>* unorderedAccessViews);

private:

	FrameLocalDescriptorHeapAllocator m_allocatorSRV;
	FrameLocalDescriptorHeapAllocator m_allocatorSampler;

	CComPtr<ID3D12Device> m_device;
	std::shared_ptr<ShaderResourceViewDx12> m_nullSrv;
	std::shared_ptr<UnorderedAccessViewDx12> m_nullUav;
	std::shared_ptr<SamplerStateDx12> m_nullSampler;

	std::shared_ptr<DescriptorHeapViewDx12> m_srvUav[Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];
	std::shared_ptr<SamplerStateDx12> m_sampler[Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];
	D3D12_GPU_VIRTUAL_ADDRESS m_cbv[Tr2RenderContextEnum::SHADER_TYPE_COUNT][Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];
	bool m_cbvDirty[Tr2RenderContextEnum::SHADER_TYPE_COUNT][Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];
	bool m_heapsDirty;
	bool m_srvUavDirty;
	bool m_samplerDirty;

	bool m_pipeDirty[Tr2RenderContextEnum::SHADER_PIPE_COUNT];

	uint32_t m_lastSrvUavParameter[Tr2RenderContextEnum::SHADER_PIPE_COUNT];
	uint32_t m_lastSamplerParameter[Tr2RenderContextEnum::SHADER_PIPE_COUNT];
	D3D12_GPU_DESCRIPTOR_HANDLE m_lastSrvUavAddress[Tr2RenderContextEnum::SHADER_PIPE_COUNT];
	D3D12_GPU_DESCRIPTOR_HANDLE m_lastSamplerAddress[Tr2RenderContextEnum::SHADER_PIPE_COUNT];
};

#endif
