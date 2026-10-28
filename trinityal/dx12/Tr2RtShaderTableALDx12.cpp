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

ALResult Tr2RtShaderTableAL::Create( const Tr2RtShaderTableDescriptionAL& desc, const ::Tr2RtPipelineStateAL& pipeline, Tr2PrimaryRenderContextAL& renderContext )
{
	if( !renderContext.IsValid() )
	{
		return E_INVALIDCALL;
	}
	Destroy();
	m_owner = &renderContext;
	m_buffers.resize( renderContext.GetBackBufferCount() );
	return Write( desc, pipeline, renderContext );
}

ALResult Tr2RtShaderTableAL::Update( const Tr2RtShaderTableDescriptionAL& desc, const ::Tr2RtPipelineStateAL& pipeline, Tr2PrimaryRenderContextAL& renderContext )
{
	if( !m_owner || m_owner != &renderContext || !renderContext.IsValid() )
	{
		return E_INVALIDCALL;
	}
	return Write( desc, pipeline, renderContext );
}

// Shader tables contain shader records. Shader records contain a shader identifier and root arguments used to look up resources
// Records are appended to a persistent upload buffer per back buffer. The write cursor only rewinds when that back buffer comes
// round again, so a table built earlier in the same frame is still intact when its DispatchRays executes
ALResult Tr2RtShaderTableAL::Write( const Tr2RtShaderTableDescriptionAL& desc, const ::Tr2RtPipelineStateAL& pipeline, Tr2PrimaryRenderContextAL& renderContext )
{
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
	const size_t size = ( desc.m_rayGenNames.size() + desc.m_missNames.size() + desc.m_hitGroupNames.size() ) * entrySize;
	const size_t alignedSize = Align( size, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT );

	auto& buffer = m_buffers[renderContext.GetCurrentBackBufferIndex()];
	const uint64_t frame = renderContext.GetRecordingFrameNumber();
	if( buffer.frame != frame )
	{
		buffer.frame = frame;
		buffer.used = 0;
	}
	if( buffer.capacity < buffer.used + alignedSize )
	{
		// Earlier tables of this frame stay alive in the retired resource until the GPU is done with it
		const size_t needed = buffer.used + alignedSize;
		const size_t capacity = Align( needed + needed / 4, 65536 );
		ReleaseBuffer( buffer );
		auto heap = HeapDesc( D3D12_HEAP_TYPE_UPLOAD );
		auto bufferDesc = BufferDesc( capacity, D3D12_RESOURCE_FLAG_NONE );
		CR_RETURN_HR( renderContext.m_device->CreateCommittedResource(
			&heap,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS( &buffer.resource ) ) );
		CR_RETURN_HR( buffer.resource->Map( 0, nullptr, (void**)&buffer.mapped ) );
		buffer.capacity = capacity;
	}

	m_staging.resize( size );
	uint8_t* tableData = m_staging.data();

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

	auto FillRecord = [&]( uint8_t* tableData, const wchar_t* name, const Tr2RtLocalMaterialDescriptionAL& material ) -> ALResult {
		auto shaderInfo = FindShaderInfo( name );
		if( !shaderInfo || !shaderInfo->shaderIdentifier )
		{
			return E_INVALIDARG;
		}
		memcpy( tableData, shaderInfo->shaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES );
		if( auto signature = shaderInfo->localSignature )
		{
			for( auto jt = begin( signature->m_cbRegisters ); jt != end( signature->m_cbRegisters ); ++jt )
			{
				auto& cb = material.m_constants[jt->index];
				D3D12_GPU_VIRTUAL_ADDRESS addr;
				if( cb && cb->IsValid() )
				{
					addr = descriptorCache.UploadConstants( *cb->TrinityALImpl_GetObject() );
				}
				else
				{
					addr = renderContext.m_nullCB.GetGpuView();
				}
				memcpy( tableData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + jt->parameter * sizeof( D3D12_GPU_VIRTUAL_ADDRESS ), &addr, sizeof( D3D12_GPU_VIRTUAL_ADDRESS ) );
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

	for( auto& record : desc.m_rayGenNames )
	{
		CR_RETURN_HR( FillRecord( tableData, record.name, record.material ) );
		tableData += entrySize;
	}

	for( auto& record : desc.m_missNames )
	{
		CR_RETURN_HR( FillRecord( tableData, record.name, record.material ) );
		tableData += entrySize;
	}

	for( auto& record : desc.m_hitGroupNames )
	{
		CR_RETURN_HR( FillRecord( tableData, record.name, record.material ) );
		tableData += entrySize;
	}

	memcpy( buffer.mapped + buffer.used, m_staging.data(), size );
	m_tableAddress = buffer.resource->GetGPUVirtualAddress() + buffer.used;
	buffer.used += alignedSize;

	m_rayGenNames.clear();
	for( auto& record : desc.m_rayGenNames )
	{
		m_rayGenNames.push_back( record.name );
	}
	m_missCount = desc.m_missNames.size();
	m_hitGroupCount = desc.m_hitGroupNames.size();
	m_entrySize = entrySize;
	return S_OK;
}

void Tr2RtShaderTableAL::ReleaseBuffer( TableBuffer& buffer )
{
	if( buffer.resource )
	{
		buffer.resource->Unmap( 0, nullptr );
		RELEASE_LATER( m_owner, buffer.resource );
		buffer.resource = nullptr;
	}
	buffer.mapped = nullptr;
	buffer.capacity = 0;
	buffer.used = 0;
}

void Tr2RtShaderTableAL::Destroy()
{
	for( auto& buffer : m_buffers )
	{
		ReleaseBuffer( buffer );
	}
	m_buffers.clear();
	m_owner = nullptr;
	m_tableAddress = 0;
	m_rayGenNames.clear();
	m_missCount = 0;
	m_hitGroupCount = 0;
	m_entrySize = 0;
}

bool Tr2RtShaderTableAL::IsValid() const
{
	return m_tableAddress != 0;
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
			return m_tableAddress + i * GetEntrySize();
		}
	}
	return 0;
}

D3D12_GPU_VIRTUAL_ADDRESS Tr2RtShaderTableAL::GetMissShaders() const
{
	return m_tableAddress + m_rayGenNames.size() * GetEntrySize();
}

D3D12_GPU_VIRTUAL_ADDRESS Tr2RtShaderTableAL::GetHitGroupShaders() const
{
	return m_tableAddress + ( m_rayGenNames.size() + m_missCount ) * GetEntrySize();
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