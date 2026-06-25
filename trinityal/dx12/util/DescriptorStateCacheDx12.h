// Copyright © 2019 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include <memory>

#include "../Tr2ConstantBufferALDx12.h"
#include "../Tr2ShaderProgramAlDx12.h"
#include "../../Tr2RenderContextEnum.h"
#include "../../include/Tr2ResourceSetAL.h"

#include "DescriptorHeapViewDx12.h"
#include "FrameLocalDescriptorHeapAllocatorDx12.h"
#include "FrameLocalUploadBufferAllocatorDx12.h"

/** State manager/cache for descriptor based bindings */
class DescriptorStateCache
{
public:
	/** */
	DescriptorStateCache( CComPtr<ID3D12Device> device, class Tr2PrimaryRenderContextAL* context );

	/** Dirty all states and reset internal allocators */
	void Reset();

	/** Dirty heap cache, this will force a call to SetDescriptorHeaps on the next Commit() */
	void Dirty();

	/** Apply state */
	void Commit( ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* globalSrvUavHeap, ID3D12DescriptorHeap* globalSamplerHeap, const TrinityALImpl::Tr2RootSignatureAL* rootSignature );

	/** Set an array of ShaderResourceViews */
	void SetShaderResources( uint32_t startSlot, uint32_t numViews, std::shared_ptr<ShaderResourceViewDx12>* shaderResourceViews );

	/** Set an array of SamplerStates */
	void SetSamplers( uint32_t startSlot, uint32_t numViews, std::shared_ptr<SamplerStateDx12>* samplers );

	/** Set a constantbuffer */
	void SetConstantBuffers( Tr2RenderContextEnum::ShaderType shaderStage, uint32_t slot, const TrinityALImpl::Tr2ConstantBufferAL& constantBuffer );
	D3D12_GPU_VIRTUAL_ADDRESS UploadConstants( const TrinityALImpl::Tr2ConstantBufferAL& constantBuffer );
	D3D12_GPU_VIRTUAL_ADDRESS UploadConstants( const void* data, uint32_t size );

	/** Set an array or UnorderedAccessViews */
	void SetUnorderedAccessViews( uint32_t startSlot, uint32_t numViews, std::shared_ptr<UnorderedAccessViewDx12>* unorderedAccessViews );

	void SetHeaps( ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* globalSrvUavHeap, ID3D12DescriptorHeap* globalSamplerHeap );

private:
	/** Instance of a root parameter set slot */
	struct RootParameterSlot
	{
		enum ERootParameterType
		{
			RPT_None = 0,
			RPT_CBV = 1,
			RPT_SRV = 2,
			RPT_Sampler = 3
		};

		RootParameterSlot()
		{
			SetNone();
		}

		/** Empty slot */
		void SetNone()
		{
			m_addressVirtual = 0;
			m_type = RPT_None;
		}

		/** Mark as SRV */
		void SetSRV( D3D12_GPU_DESCRIPTOR_HANDLE address )
		{
			m_addressDescriptor = address;
			m_type = RPT_SRV;
		}

		/** Mark as Sampler */
		void SetSampler( D3D12_GPU_DESCRIPTOR_HANDLE address )
		{
			m_addressDescriptor = address;
			m_type = RPT_Sampler;
		}

		/** Mark as CBV */
		void SetCBV( D3D12_GPU_VIRTUAL_ADDRESS address )
		{
			m_addressVirtual = address;
			m_type = RPT_CBV;
		}

		/** Does the slot contain an SRV at the given address? */
		bool IsValidSRV( D3D12_GPU_DESCRIPTOR_HANDLE address )
		{
			return m_type == RPT_SRV && m_addressDescriptor.ptr == address.ptr;
		}

		/** Does the slot contain sampler at the given address? */
		bool IsValidSampler( D3D12_GPU_DESCRIPTOR_HANDLE address )
		{
			return m_type == RPT_Sampler && m_addressDescriptor.ptr == address.ptr;
		}

		/** Does the slot contain a CBVat the given address/stage/index */
		bool IsValidCBV( D3D12_GPU_VIRTUAL_ADDRESS address )
		{
			return m_type == RPT_CBV && m_addressVirtual == address;
		}

		union
		{
			D3D12_GPU_DESCRIPTOR_HANDLE m_addressDescriptor;
			D3D12_GPU_VIRTUAL_ADDRESS m_addressVirtual;
		};

		ERootParameterType m_type;
	};

	ConstantBufferAllocator m_allocatorUpload;

	Tr2PrimaryRenderContextAL* m_primaryContext;
	CComPtr<ID3D12Device> m_device;
	std::shared_ptr<ShaderResourceViewDx12> m_nullSrv;
	std::shared_ptr<UnorderedAccessViewDx12> m_nullUav;
	std::shared_ptr<SamplerStateDx12> m_nullSampler;

	CComPtr<ID3D12RootSignature> m_rootSignature;

	std::shared_ptr<ShaderResourceViewDx12> m_srvUav[Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];
	std::shared_ptr<SamplerStateDx12> m_sampler[Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];
	D3D12_GPU_VIRTUAL_ADDRESS m_cbv[Tr2RenderContextEnum::SHADER_TYPE_COUNT][Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];

	CComPtr<ID3D12DescriptorHeap> m_globalSrvUavHeap;
	CComPtr<ID3D12DescriptorHeap> m_globalSamplerHeap;
	bool m_heapsDirty;
	bool m_srvUavDirty;
	bool m_samplerDirty;

	RootParameterSlot m_parameterSlots[Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];
};

#endif
