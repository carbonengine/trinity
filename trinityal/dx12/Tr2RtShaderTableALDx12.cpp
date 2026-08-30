// Copyright © 2019 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2RtShaderTableALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "Tr2RtPipelineStateALDx12.h"
#include "Tr2BufferALDx12.h"
#include "Tr2TextureALDx12.h"
#include "Tr2SamplerStateALDx12.h"
#include "Utilities.h"

namespace
{
uintptr_t Align( uintptr_t offset, size_t alignment )
{
	return ( offset + ( alignment - 1 ) ) & ~( alignment - 1 );
}

size_t GetSignatureSize( const TrinityALImpl::Tr2RootSignatureAL* signature )
{
	if( !signature )
	{
		return 0;
	}
	size_t size = signature->m_cbRegisters.size() * sizeof( D3D12_GPU_DESCRIPTOR_HANDLE );
	size += signature->m_srvRegisters.size() * sizeof( D3D12_GPU_DESCRIPTOR_HANDLE );
	size += signature->m_uavRegisters.size() * sizeof( D3D12_GPU_DESCRIPTOR_HANDLE );
	size += signature->m_samplerRegisters.size() * sizeof( D3D12_GPU_DESCRIPTOR_HANDLE );
	return size;
}

size_t GetMaxElementSize( const ::Tr2RtPipelineStateAL& pipeline )
{
	size_t signatureSize = 0;
	auto& localSignatures = pipeline.TrinityALImpl_GetObject()->GetLocalSignatures();
	for( auto it = begin( localSignatures ); it != end( localSignatures ); ++it )
	{
		signatureSize = std::max( signatureSize, GetSignatureSize( *it ) );
	}
	return signatureSize;
}
}


namespace TrinityALImpl
{
Tr2RtShaderTableAL::Tr2RtShaderTableAL() :
	m_owner( nullptr ),
	m_entrySize( 0 )
{
}

Tr2RtShaderTableAL::~Tr2RtShaderTableAL()
{
	Destroy();
}

// Shader tables contain shader records. Shader records contain a shader identifier and root arguments used to look up resources.
// Layout: retained hit groups, per-frame hit groups, ray generation, miss. Retained records keep their offsets across frames
// and only the ones added since this buffer was last written are filled in.
ALResult Tr2RtShaderTableAL::Create( const Tr2RtShaderTableDescriptionAL& desc, const ::Tr2RtPipelineStateAL& pipeline, Tr2PrimaryRenderContextAL& renderContext )
{
	if( !renderContext.IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( !pipeline.IsValid() )
	{
		return E_INVALIDARG;
	}

	// Gather info for local signature
	// local signature contains material(s) and sampler(s)
	size_t signatureSize = 0;
	uint32_t srvDescriptorCount = 0;
	uint32_t samplerDescriptorCount = 0;
	for( auto& signature : pipeline.TrinityALImpl_GetObject()->GetLocalSignatures() )
	{
		signatureSize = std::max( signatureSize, GetSignatureSize( signature ) );
		srvDescriptorCount = std::max( srvDescriptorCount, signature->m_srvUavParameterCount );
		samplerDescriptorCount = std::max( samplerDescriptorCount, signature->m_samplerParameterCount );
	}

	auto& descriptorCache = *renderContext.m_descriptorCache[renderContext.GetCurrentBackBufferIndex()];

	const size_t entrySize = Align( D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + signatureSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT );
	const size_t retainedCount = desc.m_retainedHitGroups.size();
	const size_t hitGroupCount = retainedCount + desc.m_hitGroupNames.size();
	const size_t size = ( hitGroupCount + desc.m_rayGenNames.size() + desc.m_missNames.size() ) * entrySize;

	const bool reuseBuffers = ::Tr2RtShaderTableAL::s_reuseBuffers;
	if( m_owner && ( m_owner != &renderContext || !reuseBuffers ) )
	{
		Destroy();
	}

	ID3D12StateObject* stateObject = pipeline.TrinityALImpl_GetObject()->GetStateObject();
	CComPtr<ID3D12Resource> table;
	uint8_t* tableData = nullptr;
	size_t firstRetained = 0;
	TableBuffer* buffer = nullptr;
	if( reuseBuffers )
	{
		m_owner = &renderContext;
		if( m_buffers.empty() )
		{
			if( m_table )
			{
				RELEASE_LATER( m_owner, m_table );
				m_table = nullptr;
			}
			m_buffers.resize( renderContext.GetBackBufferCount() );
		}
		buffer = &m_buffers[renderContext.GetCurrentBackBufferIndex()];
		const size_t alignedSize = Align( size, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT );
		if( buffer->capacity < alignedSize )
		{
			if( buffer->resource )
			{
				buffer->resource->Unmap( 0, nullptr );
				RELEASE_LATER( m_owner, buffer->resource );
				buffer->resource = nullptr;
				buffer->mapped = nullptr;
				buffer->capacity = 0;
			}
			const size_t capacity = Align( alignedSize + alignedSize / 4, 65536 );
			auto heap = HeapDesc( D3D12_HEAP_TYPE_UPLOAD );
			auto bufferDesc = BufferDesc( capacity, D3D12_RESOURCE_FLAG_NONE );
			CR_RETURN_HR( renderContext.m_device->CreateCommittedResource(
				&heap,
				D3D12_HEAP_FLAG_NONE,
				&bufferDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS( &buffer->resource ) ) );
			CR_RETURN_HR( buffer->resource->Map( 0, nullptr, (void**)&buffer->mapped ) );
			buffer->capacity = capacity;
			buffer->retainedWritten = 0;
		}
		if( buffer->pipeline != stateObject || buffer->retainedVersion != desc.m_retainedVersion || buffer->entrySize != entrySize )
		{
			buffer->retainedWritten = 0;
		}
		table = buffer->resource;
		tableData = buffer->mapped;
		firstRetained = std::min<size_t>( buffer->retainedWritten, retainedCount );
	}
	else
	{
		auto heap = HeapDesc( D3D12_HEAP_TYPE_UPLOAD );
		auto scratchDesc = BufferDesc( Align( size, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT ), D3D12_RESOURCE_FLAG_NONE );
		CR_RETURN_HR( renderContext.m_device->CreateCommittedResource(
			&heap,
			D3D12_HEAP_FLAG_NONE,
			&scratchDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS( &table ) ) );
		CR_RETURN_HR( table->Map( 0, nullptr, (void**)&tableData ) );
	}
	ON_BLOCK_EXIT( [&] { if( !reuseBuffers ) table->Unmap( 0, nullptr ); } );

	struct CachedShaderInfo
	{
		const wchar_t* name;
		const TrinityALImpl::Tr2RtPipelineStateAL::ShaderInfo* info;
	};
	std::vector<CachedShaderInfo> shaderInfos;
	auto FindShaderInfo = [&]( const wchar_t* name ) {
		for( auto& cached : shaderInfos )
		{
			if( cached.name == name )
			{
				return cached.info;
			}
		}
		auto info = pipeline.TrinityALImpl_GetObject()->GetShaderInfo( name );
		shaderInfos.push_back( { name, info } );
		return info;
	};

	auto FillRecord = [&]( uint8_t* record, const wchar_t* name, const Tr2RtLocalMaterialDescriptionAL& material ) -> ALResult {
		auto shaderInfo = FindShaderInfo( name );
		if( !shaderInfo || !shaderInfo->shaderIdentifier )
		{
			return E_INVALIDARG;
		}
		memcpy( record, shaderInfo->shaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES );
		if( auto signature = shaderInfo->localSignature )
		{
			for( auto& cbRegister : signature->m_cbRegisters )
			{
				auto& constant = material.m_constants[cbRegister.index];
				D3D12_GPU_VIRTUAL_ADDRESS addr;
				if( constant.address )
				{
					addr = constant.address;
				}
				else if( constant.buffer && constant.buffer->IsValid() )
				{
					addr = descriptorCache.UploadConstants( *constant.buffer->TrinityALImpl_GetObject() );
				}
				else
				{
					addr = renderContext.m_nullCB.GetGpuView();
				}
				memcpy( record + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + cbRegister.parameter * sizeof( D3D12_GPU_VIRTUAL_ADDRESS ), &addr, sizeof( D3D12_GPU_VIRTUAL_ADDRESS ) );
			}
			if( signature->m_srvUavParameterCount )
			{
				return E_FAIL;
			}
			if( signature->m_samplerParameterCount )
			{
				return E_FAIL;
			}
		}
		return S_OK;
	};

	for( size_t i = firstRetained; i < retainedCount; ++i )
	{
		auto& retained = desc.m_retainedHitGroups[i];
		CR_RETURN_HR( FillRecord( tableData + i * entrySize, retained.name, retained.material ) );
	}
	uint8_t* record = tableData + retainedCount * entrySize;
	for( auto& it : desc.m_hitGroupNames )
	{
		CR_RETURN_HR( FillRecord( record, it.name, it.material ) );
		record += entrySize;
	}
	for( auto& it : desc.m_rayGenNames )
	{
		CR_RETURN_HR( FillRecord( record, it.name, it.material ) );
		record += entrySize;
	}
	for( auto& it : desc.m_missNames )
	{
		CR_RETURN_HR( FillRecord( record, it.name, it.material ) );
		record += entrySize;
	}

	if( buffer )
	{
		buffer->retainedWritten = uint32_t( retainedCount );
		buffer->retainedVersion = desc.m_retainedVersion;
		buffer->pipeline = stateObject;
		buffer->entrySize = entrySize;
	}
	else
	{
		m_desc = desc;
	}
	m_rayGenNames.clear();
	for( auto& it : desc.m_rayGenNames )
	{
		m_rayGenNames.push_back( it.name );
	}
	m_missCount = desc.m_missNames.size();
	m_hitGroupCount = hitGroupCount;
	m_table = table;
	m_entrySize = entrySize;
	m_owner = &renderContext;
	return S_OK;
}

void Tr2RtShaderTableAL::ReleaseBuffers()
{
	for( auto& buffer : m_buffers )
	{
		if( buffer.resource )
		{
			buffer.resource->Unmap( 0, nullptr );
			RELEASE_LATER( m_owner, buffer.resource );
		}
	}
	m_buffers.clear();
}

void Tr2RtShaderTableAL::Destroy()
{
	if( m_owner )
	{
		if( m_buffers.empty() )
		{
			RELEASE_LATER( m_owner, m_table );
		}
		ReleaseBuffers();
		m_owner = nullptr;
		m_table = nullptr;
		m_desc = Tr2RtShaderTableDescriptionAL();
		m_rayGenNames.clear();
		m_missCount = 0;
		m_hitGroupCount = 0;
		m_entrySize = 0;
	}
}

bool Tr2RtShaderTableAL::IsValid() const
{
	return m_table != nullptr;
}

Tr2ALMemoryType Tr2RtShaderTableAL::GetMemoryClass() const
{
	return AL_MEMORY_MANAGED;
}

void Tr2RtShaderTableAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2RtShaderTableAL";
}

D3D12_GPU_VIRTUAL_ADDRESS Tr2RtShaderTableAL::GetRayGenShader( const wchar_t* name ) const
{
	for( size_t i = 0; i < m_rayGenNames.size(); ++i )
	{
		if( m_rayGenNames[i] == name )
		{
			return m_table->GetGPUVirtualAddress() + ( m_hitGroupCount + i ) * GetEntrySize();
		}
	}
	return 0;
}

D3D12_GPU_VIRTUAL_ADDRESS Tr2RtShaderTableAL::GetMissShaders() const
{
	return m_table->GetGPUVirtualAddress() + ( m_hitGroupCount + m_rayGenNames.size() ) * GetEntrySize();
}

D3D12_GPU_VIRTUAL_ADDRESS Tr2RtShaderTableAL::GetHitGroupShaders() const
{
	return m_table->GetGPUVirtualAddress();
}

uint64_t Tr2RtShaderTableAL::GetEntrySize() const
{
	return m_entrySize;
}

uint64_t Tr2RtShaderTableAL::GetMissShaderTableSize() const
{
	return GetEntrySize() * m_missCount;
}

uint64_t Tr2RtShaderTableAL::GetHitGroupTableSize() const
{
	return GetEntrySize() * m_hitGroupCount;
}
}


#endif