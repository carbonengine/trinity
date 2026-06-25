// Copyright © 2019 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2RtBottomLevelAccelerationStructureALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "Tr2BufferALDx12.h"
#include "Utilities.h"

namespace
{
uintptr_t Align( uintptr_t offset, size_t alignment )
{
	return ( offset + ( alignment - 1 ) ) & ~( alignment - 1 );
}

D3D12_RAYTRACING_GEOMETRY_DESC CreateGeometryDesc( const Tr2RtGeometryAL& geometry, Tr2RtBlasGeometryFlags::Type geometryFlags )
{
	D3D12_RAYTRACING_GEOMETRY_DESC result;
	result.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	result.Triangles.VertexBuffer.StartAddress = geometry.positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetGpuView() +
		geometry.positions.m_vertexOffset * geometry.positions.m_stride + geometry.positions.m_positionOffset;
	result.Triangles.VertexBuffer.StrideInBytes = geometry.positions.m_stride;
	result.Triangles.VertexCount = geometry.positions.m_vertexCount;
	result.Triangles.VertexFormat = DXGI_FORMAT( geometry.positions.m_positionFormat );
	result.Triangles.IndexBuffer = geometry.indices.m_indexBuffer.TrinityALImpl_GetObject()->GetGpuView() + geometry.indices.m_stride * geometry.indices.m_indexOffset;
	result.Triangles.IndexFormat = geometry.indices.m_stride == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	result.Triangles.IndexCount = geometry.indices.m_indexCount;
	result.Triangles.Transform3x4 = 0;
	result.Flags = geometryFlags ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
	return result;
}

D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS CreateInputsDesc( const D3D12_RAYTRACING_GEOMETRY_DESC* geometry, Tr2RtBuildFlags::Type buildFlags )
{
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS desc = {};
	desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY; //For a data set of n elements, the pointer parameter simply points to the start of an of n elements in memory.
	desc.pGeometryDescs = geometry;
	desc.NumDescs = 1;
	desc.Flags = static_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS>( buildFlags );
	return desc;
}

void BuildRaytracingAccelerationStructure( const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& desc, const Tr2RtGeometryAL& geometry, Tr2RenderContextAL& renderContext )
{
	D3D12_RESOURCE_BARRIER barriers[2];
	uint32_t barrierCount = 0;
	if( ( geometry.positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetDefaultState() & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE ) == 0 )
	{
		barriers[barrierCount++] = TrinityALImpl::Transition( geometry.positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetGpuResource(), geometry.positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetDefaultState(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE );
	}
	if( geometry.positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetGpuResource() != geometry.indices.m_indexBuffer.TrinityALImpl_GetObject()->GetGpuResource() &&
		( geometry.indices.m_indexBuffer.TrinityALImpl_GetObject()->GetDefaultState() & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE ) == 0 )
	{
		barriers[barrierCount++] = TrinityALImpl::Transition( geometry.indices.m_indexBuffer.TrinityALImpl_GetObject()->GetGpuResource(), geometry.indices.m_indexBuffer.TrinityALImpl_GetObject()->GetDefaultState(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE );
	}
	if( barrierCount )
	{
		renderContext.ResourceBarrierDx12( barrierCount, barriers );
	}
	renderContext.FlushBarriersDx12( geometry.positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetGpuResource(), geometry.indices.m_indexBuffer.TrinityALImpl_GetObject()->GetGpuResource() );

	// BuildRaytracingAccelerationStructure function runs on gpu and returns size of structure
	renderContext.m_commandList4->BuildRaytracingAccelerationStructure( &desc, 0, nullptr );


	std::swap( barriers[0].Transition.StateAfter, barriers[0].Transition.StateBefore );
	std::swap( barriers[1].Transition.StateAfter, barriers[1].Transition.StateBefore );
	renderContext.ResourceBarrierDx12( barrierCount, barriers );
}

}

namespace TrinityALImpl
{
Tr2RtBottomLevelAccelerationStructureAL::Tr2RtBottomLevelAccelerationStructureAL() :
	m_bufferAddress{},
	m_geometryDesc{},
	m_geometryFlags{},
	m_buildFlags{},
	m_owner( nullptr )
{
}

Tr2RtBottomLevelAccelerationStructureAL::~Tr2RtBottomLevelAccelerationStructureAL()
{
	Destroy();
}

ALResult Tr2RtBottomLevelAccelerationStructureAL::Create( const Tr2RtGeometryAL& geometry, const Tr2RtGeometryAL& capacity, Tr2RtBlasGeometryFlags::Type geometryFlags, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext )
{
	if( !renderContext.IsValid() || !renderContext.GetCaps().SupportsRaytracing() )
	{
		return E_INVALIDCALL;
	}
	if( !geometry.positions.IsValid() || !HasFlag( geometry.positions.m_vertexBuffer.GetDesc().gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
	{
		return E_INVALIDARG;
	}
	if( !geometry.indices.IsValid() || !HasFlag( geometry.indices.m_indexBuffer.GetDesc().gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
	{
		return E_INVALIDARG;
	}
	if( !renderContext.m_commandList4 )
	{
		return E_INVALIDCALL;
	}

	// gather geometry info for buffer
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = CreateGeometryDesc( capacity, geometryFlags );

	// size requirements for scratch and acceleration structure buffers
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS prebuildDesc = CreateInputsDesc( &geometryDesc, buildFlags );

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO preBuildInfo = {};
	// do some bvh optimization and return an estimate on how big the structure will be - runs on cpu
	renderContext.m_device5->GetRaytracingAccelerationStructurePrebuildInfo( &prebuildDesc, &preBuildInfo );

	if( preBuildInfo.ResultDataMaxSizeInBytes <= 0 )
	{
		return E_INVALIDCALL;
	}

	// Create two buffers
	// 1.	Scratch buffer which is required for intermediate computation.
	// 2.	The result buffer which will hold the acceleration data.
	CComPtr<ID3D12Resource> scratch;
	CComPtr<ID3D12Resource> buffer;

	auto heap = TrinityALImpl::HeapDesc( D3D12_HEAP_TYPE_DEFAULT );

	// Buffer sizes need to be 256-byte-aligned
	auto scratchDesc = TrinityALImpl::BufferDesc( Align( preBuildInfo.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT ), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS );
	CR_RETURN_HR( renderContext.m_device->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&scratchDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS( &scratch ) ) );

	// for no compaction we use ResultDataMaxSizeInBytes
	auto bufferDesc = TrinityALImpl::BufferDesc( Align( preBuildInfo.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT ), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS );
	CR_RETURN_HR( renderContext.m_device->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nullptr,
		IID_PPV_ARGS( &buffer ) ) );

	geometryDesc = CreateGeometryDesc( geometry, geometryFlags );

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
	desc.Inputs = prebuildDesc;
	desc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
	desc.DestAccelerationStructureData = buffer->GetGPUVirtualAddress();

	BuildRaytracingAccelerationStructure( desc, geometry, renderContext );

	m_buffer = buffer;
	m_owner = &renderContext;
	m_geometryDesc = geometryDesc;
	m_geometryFlags = geometryFlags;
	m_buildFlags = buildFlags;
	if( ( buildFlags & Tr2RtBuildFlags::ALLOW_UPDATE ) != 0 )
	{
		m_scratch = scratch;
	}
	else
	{
		renderContext.ReleaseLater( scratch );
	}
	m_bufferAddress = m_buffer->GetGPUVirtualAddress();

	return S_OK;
}

ALResult Tr2RtBottomLevelAccelerationStructureAL::Update( const Tr2RtGeometryAL& geometry, Tr2RenderContextAL& renderContext )
{
	if( !m_scratch || !m_buffer )
	{
		return E_INVALIDCALL;
	}
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
	if( !renderContext.m_commandList4 )
	{
		return E_INVALIDCALL;
	}

	bool rebuild = false;

	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = CreateGeometryDesc( geometry, m_geometryFlags );
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = CreateInputsDesc( &geometryDesc, m_buildFlags );

	if( m_geometryDesc.Triangles.VertexBuffer.StrideInBytes != geometryDesc.Triangles.VertexBuffer.StrideInBytes ||
		m_geometryDesc.Triangles.VertexCount != geometryDesc.Triangles.VertexCount ||
		m_geometryDesc.Triangles.VertexFormat != geometryDesc.Triangles.VertexFormat ||
		m_geometryDesc.Triangles.IndexFormat != geometryDesc.Triangles.IndexFormat ||
		m_geometryDesc.Triangles.IndexCount != geometryDesc.Triangles.IndexCount )
	{

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO preBuildInfo = {};
		renderContext.GetPrimaryRenderContext().m_device5->GetRaytracingAccelerationStructurePrebuildInfo( &inputs, &preBuildInfo );

		if( preBuildInfo.ResultDataMaxSizeInBytes <= 0 || preBuildInfo.ScratchDataSizeInBytes > m_scratch->GetDesc().Width || preBuildInfo.ResultDataMaxSizeInBytes > m_buffer->GetDesc().Width )
		{
			return E_INVALIDARG;
		}
		rebuild = true;
	}
	m_geometryDesc = geometryDesc;

	if( !rebuild )
	{
		inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
	desc.Inputs = inputs;
	desc.SourceAccelerationStructureData = rebuild ? 0 : m_buffer->GetGPUVirtualAddress(); //  If this address is the same as DestAccelerationStructureData, the update is to be performed in-place.
	desc.ScratchAccelerationStructureData = m_scratch->GetGPUVirtualAddress();
	desc.DestAccelerationStructureData = m_buffer->GetGPUVirtualAddress();

	BuildRaytracingAccelerationStructure( desc, geometry, renderContext );

	return S_OK;
}

ALResult Tr2RtBottomLevelAccelerationStructureAL::CompactBlas( Tr2PrimaryRenderContextAL& renderContext )
{
	/**************** COMPACTION ****************/

	// full compaction:
	// for each acceleration structure
	//
	//	    BuildRaytracingAccelerationStructure to (tempBuffer + tempOffset )
	//      tempOffset += _PREBUILD_INFO.ResultDataMaxSizeInBytes
	//  wait for builds to finish executing
	//  maybe even do some rendering
	//
	// For each acceleration structure
	//       CopyRayTracingAccelerationStructure(buffer + bufferOffset,
	//											tempBufferLocation, _ACCELERATION_STRUCTURE_COPY_MODE_COMPACT);
	//	    bufferOffset += _POSTBUILD_INFO_COMPACTED_SIZE
	/*
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postBuildDesc = {};
		postBuildDesc.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;

		CComPtr<ID3D12Resource> finalBuffer;
		// Create a new target BLAS resource with the known compaction size. Now you know the size and you can make your target BLAS resource ready.
		auto tempBuffer = TrinityALImpl::BufferDesc( Align( postBuildDesc.DestBuffer, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT ), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS );
		CR_RETURN_HR( renderContext.m_device->CreateCommittedResource(
			&heap,
			D3D12_HEAP_FLAG_NONE,
			&tempBuffer,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
			nullptr,
			IID_PPV_ARGS( &finalBuffer ) ) );

		// instead of this we could just call BuildRaytracingAccelerationStructure but with the postBuild structure
		// for that we wouldn't need a barrier between build completed and requesting postbuild info
		renderContext.m_commandList4->EmitRaytracingAccelerationStructurePostbuildInfo( &postBuildDesc, 0, nullptr);

		auto heap = TrinityALImpl::HeapDesc( D3D12_HEAP_TYPE_DEFAULT );

	


		// copy from the source BLAS to the target BLAS
		renderContext.m_commandList4->CopyRaytracingAccelerationStructure( postBuildDesc.DestBuffer, finalBuffer->GetGPUVirtualAddress(), D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT );

		m_buffer = finalBuffer;*/

	return S_OK;
}

void Tr2RtBottomLevelAccelerationStructureAL::Destroy()
{
	if( m_owner )
	{
		RELEASE_LATER( m_owner, m_buffer );
		if( m_scratch )
		{
			RELEASE_LATER( m_owner, m_scratch );
		}
		m_scratch = nullptr;
		m_buffer = nullptr;
		m_bufferAddress = {};
		m_geometryDesc = {};
		m_owner = nullptr;
	}
}

bool Tr2RtBottomLevelAccelerationStructureAL::IsValid() const
{
	return m_buffer != nullptr;
}

ID3D12Resource* Tr2RtBottomLevelAccelerationStructureAL::GetBuffer() const
{
	return m_buffer;
}

D3D12_GPU_VIRTUAL_ADDRESS Tr2RtBottomLevelAccelerationStructureAL::GetGPUVirtualAddress() const
{
	return m_bufferAddress;
}

Tr2ALMemoryType Tr2RtBottomLevelAccelerationStructureAL::GetMemoryClass() const
{
	return AL_MEMORY_MANAGED;
}

void Tr2RtBottomLevelAccelerationStructureAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2RtBottomLevelAccelerationStructureAL";
}
}
#endif
