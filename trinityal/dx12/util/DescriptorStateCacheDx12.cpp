// Copyright © 2019 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "DescriptorStateCacheDx12.h"

#include "../Tr2PrimaryRenderContextDx12.h"

/** */
DescriptorStateCache::DescriptorStateCache( CComPtr<ID3D12Device> device, Tr2PrimaryRenderContextAL* context ) :
	m_primaryContext( context ),
	m_device( device )
{
	m_allocatorUpload.Initialize( device );

	D3D12_SHADER_RESOURCE_VIEW_DESC nullSrvDesc = {};
	nullSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	nullSrvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	nullSrvDesc.Texture2D.MipLevels = 1;
	context->CreateShaderResourceView( nullptr, nullSrvDesc, m_nullSrv );

	D3D12_UNORDERED_ACCESS_VIEW_DESC nullUavDesc = {};
	nullUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	nullUavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	context->CreateUnorderedAccessView( nullptr, nullptr, nullUavDesc, m_nullUav );

	D3D12_SAMPLER_DESC nullSamplerDesc = { D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0, 0, D3D12_COMPARISON_FUNC_ALWAYS };
	nullSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	nullSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	nullSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	nullSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	nullSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	context->CreateSamplerState( nullSamplerDesc, m_nullSampler );

	Reset();
}

/** Dirty all states and reset internal allocators */
void DescriptorStateCache::Reset()
{
	for( uint32_t slot = 0; slot < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++slot )
	{
		m_srvUav[slot] = m_nullSrv;
		m_sampler[slot] = m_nullSampler;

		m_parameterSlots[slot].SetNone();
	}
	memset( m_cbv, 0, sizeof( m_cbv ) );

	m_heapsDirty = true;
	m_srvUavDirty = true;
	m_samplerDirty = true;

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

	// Pretend that nothing is currently bound forcing the next Commit() to re-assign every parameter
	for( uint32_t slot = 0; slot < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++slot )
	{
		m_parameterSlots[slot].SetNone();
		m_parameterSlots[slot].SetNone();
	}
}

/** Set an array of ShaderResourceViews */
void DescriptorStateCache::SetShaderResources( uint32_t startSlot, uint32_t numViews, std::shared_ptr<ShaderResourceViewDx12>* shaderResourceViews )
{
	for( uint32_t slot = 0; slot < numViews; ++slot )
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
void DescriptorStateCache::SetUnorderedAccessViews( uint32_t startSlot, uint32_t numViews, std::shared_ptr<UnorderedAccessViewDx12>* unorderedAccessViews )
{
	for( uint32_t slot = 0; slot < numViews; ++slot )
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
void DescriptorStateCache::SetSamplers( uint32_t startSlot, uint32_t numViews, std::shared_ptr<SamplerStateDx12>* samplers )
{
	for( uint32_t slot = 0; slot < numViews; ++slot )
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


void DescriptorStateCache::SetHeaps( ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* globalSrvUavHeap, ID3D12DescriptorHeap* globalSamplerHeap )
{
	if( m_heapsDirty || m_globalSrvUavHeap != globalSrvUavHeap || m_globalSamplerHeap != globalSamplerHeap )
	{
		ID3D12DescriptorHeap* heaps[] = {
			globalSrvUavHeap,
			globalSamplerHeap
		};

		commandList->SetDescriptorHeaps( 2, heaps );

		m_globalSrvUavHeap = globalSrvUavHeap;
		m_globalSamplerHeap = globalSamplerHeap;
		m_heapsDirty = false;
		m_srvUavDirty = true;
		m_samplerDirty = true;
	}
}

/** Apply state */
void DescriptorStateCache::Commit( ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* globalSrvUavHeap, ID3D12DescriptorHeap* globalSamplerHeap, const TrinityALImpl::Tr2RootSignatureAL* rootSignature )
{
	bool rootSignatureChanged = rootSignature->m_rootSignature != m_rootSignature;

	// If we are changing the root signature or rebinding heaps, we must rebind everything
	bool forceSet = rootSignatureChanged || m_heapsDirty || m_globalSrvUavHeap != globalSrvUavHeap || m_globalSamplerHeap != globalSamplerHeap;

	SetHeaps( commandList, globalSrvUavHeap, globalSamplerHeap );

	bool mustBindSrvUav = m_srvUavDirty || forceSet;
	bool mustBindSampler = m_samplerDirty || forceSet;

	auto setRootDescriptorTable = rootSignature->m_isCompute ? &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable : &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable;

	// Check if the SRV/UAV table *can* be bound, *must* be bound and isn't already bound
	if( mustBindSrvUav )
	{
		for( uint32_t i = 0; i < rootSignature->m_srvUavParameterCount; ++i )
		{
			uint32_t index = rootSignature->m_srvUavParameterOffset + i;
			auto handle = m_srvUav[index]->GetHandleGPU();
			if( forceSet || !m_parameterSlots[index].IsValidSRV( handle ) )
			{
				m_parameterSlots[index].SetSRV( handle );
				( commandList->*setRootDescriptorTable )( index, handle );
			}
		}
	}

	if( mustBindSampler )
	{
		for( uint32_t i = 0; i < rootSignature->m_samplerParameterCount; ++i )
		{
			uint32_t index = rootSignature->m_samplerParameterOffset + i;
			auto handle = m_sampler[index]->GetHandleGPU();
			if( forceSet || !m_parameterSlots[index].IsValidSampler( handle ) )
			{
				m_parameterSlots[index].SetSampler( handle );
				( commandList->*setRootDescriptorTable )( index, handle );
			}
		}
	}

	D3D12_GPU_VIRTUAL_ADDRESS nullCbv = m_primaryContext->m_nullCB.GetGpuView();
	auto setConstantBufferView = rootSignature->m_isCompute ? &ID3D12GraphicsCommandList::SetComputeRootConstantBufferView : &ID3D12GraphicsCommandList::SetGraphicsRootConstantBufferView;
	for( auto it = begin( rootSignature->m_cbRegisters ); it != end( rootSignature->m_cbRegisters ); ++it )
	{
		D3D12_GPU_VIRTUAL_ADDRESS address = m_cbv[it->stage][it->index];
		if( rootSignatureChanged || !m_parameterSlots[it->parameter].IsValidCBV( address ) )
		{
			m_parameterSlots[it->parameter].SetCBV( address );
			( commandList->*setConstantBufferView )( it->parameter, address != 0 ? address : nullCbv );
		}
	}

	// Clear dirty flags
	m_srvUavDirty = false;
	m_samplerDirty = false;
	if( rootSignatureChanged )
	{
		m_rootSignature = rootSignature->m_rootSignature;
	}
}

#endif
