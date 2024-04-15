////////////////////////////////////////////////////////////
//
//    Created:   November 2019
//    Copyright: CCP 2019
//

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
		return (offset + (alignment - 1)) & ~(alignment - 1);
	}

}

namespace TrinityALImpl
{
	Tr2RtBottomLevelAccelerationStructureAL::Tr2RtBottomLevelAccelerationStructureAL()
		:m_owner( nullptr )
	{
	}

	Tr2RtBottomLevelAccelerationStructureAL::~Tr2RtBottomLevelAccelerationStructureAL()
	{
		Destroy();
	}

	ALResult Tr2RtBottomLevelAccelerationStructureAL::Create( const Tr2RtPositionStreamAL& positions, const Tr2RtIndicesStreamAL& indices, int numObjects, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext )
	{
		if( !renderContext.IsValid() || !renderContext.GetCaps().SupportsRaytracing() )
		{
			return E_INVALIDCALL;
		}
		if( !positions.IsValid() || !HasFlag( positions.m_vertexBuffer.GetDesc().gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
		{
			return E_INVALIDARG;
		}
		if( !indices.IsValid() || !HasFlag( indices.m_indexBuffer.GetDesc().gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
		{
			return E_INVALIDARG;
		}
		
		// gather geometry info for buffer
		D3D12_RAYTRACING_GEOMETRY_DESC geometry;
		geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geometry.Triangles.VertexBuffer.StartAddress = positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetGpuView() +
			positions.m_vertexOffset * positions.m_stride + positions.m_positionOffset;
		geometry.Triangles.VertexBuffer.StrideInBytes = positions.m_stride;
		geometry.Triangles.VertexCount = positions.m_vertexCount;
		geometry.Triangles.VertexFormat = DXGI_FORMAT( positions.m_positionFormat );
		geometry.Triangles.IndexBuffer = indices.m_indexBuffer.TrinityALImpl_GetObject()->GetGpuView() + indices.m_stride * indices.m_indexOffset;
		geometry.Triangles.IndexFormat = indices.m_stride == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		geometry.Triangles.IndexCount = indices.m_indexCount;
		geometry.Triangles.Transform3x4 = 0;
		geometry.Flags = Tr2RtBlasGeometryFlags::OPAQUE_GEOMETRY ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
		
		
		// size requirements for scratch and acceleration structure buffers
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS prebuildDesc = {};
		prebuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		prebuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY; //For a data set of n elements, the pointer parameter simply points to the start of an of n elements in memory.
		prebuildDesc.pGeometryDescs = &geometry;
		prebuildDesc.NumDescs = numObjects;
		prebuildDesc.Flags = (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS)buildFlags;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO preBuildInfo = {};
		// do some bvh optimization and return an estimate on how big the structure will be - runs on cpu
		renderContext.m_device5->GetRaytracingAccelerationStructurePrebuildInfo( &prebuildDesc, &preBuildInfo );

		if( preBuildInfo.ResultDataMaxSizeInBytes <= 0 )
		{
			CCP_LOGERR( "No data in BLAS" );
			return E_INVALIDCALL;
		}

		// Create two buffers 
		// 1.	Scratch buffer which is required for intermediate computation.
		// 2.	The result buffer which will hold the acceleration data.
		CComPtr<ID3D12Resource> scratch, buffer;
		
		// upload heap?
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


		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
		desc.Inputs = prebuildDesc;

		desc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
		desc.DestAccelerationStructureData = buffer->GetGPUVirtualAddress();

		D3D12_RESOURCE_BARRIER barriers[2];
		uint32_t barrierCount = 0;
		if( ( positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetDefaultState() & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE ) == 0 )
		{
			barriers[barrierCount++] = TrinityALImpl::Transition( positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetGpuResource(), positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetDefaultState(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE );
		}
		if( positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetGpuResource() != indices.m_indexBuffer.TrinityALImpl_GetObject()->GetGpuResource() &&
			( indices.m_indexBuffer.TrinityALImpl_GetObject()->GetDefaultState() & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE ) == 0 )
		{
			barriers[barrierCount++] = TrinityALImpl::Transition( indices.m_indexBuffer.TrinityALImpl_GetObject()->GetGpuResource(), indices.m_indexBuffer.TrinityALImpl_GetObject()->GetDefaultState(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE );
		}
		if( barrierCount )
		{
			renderContext.ResourceBarrierDx12( barrierCount, barriers );
		}
		renderContext.FlushBarriersDx12( positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetGpuResource(), indices.m_indexBuffer.TrinityALImpl_GetObject()->GetGpuResource() );

		// BuildRaytracingAccelerationStructure function runs on gpu and returns size of structure
		renderContext.m_commandList4->BuildRaytracingAccelerationStructure( &desc, 0, nullptr );

		std::swap( barriers[0].Transition.StateAfter, barriers[0].Transition.StateBefore );
		std::swap( barriers[1].Transition.StateAfter, barriers[1].Transition.StateBefore );
		renderContext.ResourceBarrierDx12( barrierCount, barriers );

		m_buffer = buffer;
		m_owner = &renderContext;
		m_geometryDesc = geometry;
		m_buildFlags = buildFlags;
		if( (buildFlags & Tr2RtBuildFlags::ALLOW_UPDATE) != 0 )
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

	ALResult Tr2RtBottomLevelAccelerationStructureAL::Update( const Tr2RtPositionStreamAL& positions, const Tr2RtIndicesStreamAL& indices, Tr2RenderContextAL& renderContext )
	{
		if( !m_scratch )
		{
			return E_INVALIDCALL;
		}
		if( !renderContext.IsValid() )
		{
			return E_INVALIDARG;
		}
		if( m_geometryDesc.Triangles.VertexBuffer.StrideInBytes	!= positions.m_stride													||
			m_geometryDesc.Triangles.VertexCount				!= positions.m_vertexCount																		||
			m_geometryDesc.Triangles.VertexFormat				!= DXGI_FORMAT( positions.m_positionFormat )													||
			m_geometryDesc.Triangles.IndexFormat				!= (indices.m_stride == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT)	||
			m_geometryDesc.Triangles.IndexCount					!= indices.m_indexCount )
		{
			return E_INVALIDARG;
		}

		D3D12_RAYTRACING_GEOMETRY_DESC geometry = m_geometryDesc;
		geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geometry.Triangles.VertexBuffer.StartAddress = positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetGpuView() +
			positions.m_vertexOffset * positions.m_stride + positions.m_positionOffset;
		geometry.Triangles.IndexBuffer = indices.m_indexBuffer.TrinityALImpl_GetObject()->GetGpuView() + indices.m_stride * indices.m_indexOffset;

		// get size requirements for scratch and acceleration structure buffers
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY; //For a data set of n elements, the pointer parameter simply points to the start of an of n elements in memory.
		inputs.pGeometryDescs = &geometry;
		inputs.NumDescs = 1; // number of elements pGeometryDescs refer to
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE | ((D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS)m_buildFlags);

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
		desc.Inputs = inputs;
		desc.SourceAccelerationStructureData = m_buffer->GetGPUVirtualAddress(); //  If this address is the same as DestAccelerationStructureData, the update is to be performed in-place.
		desc.ScratchAccelerationStructureData = m_scratch->GetGPUVirtualAddress();
		desc.DestAccelerationStructureData = m_buffer->GetGPUVirtualAddress();

		D3D12_RESOURCE_BARRIER barriers[2];
		uint32_t barrierCount = 0;
		if( ( positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetDefaultState() & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE ) == 0 )
		{
			barriers[barrierCount++] = TrinityALImpl::Transition( positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetGpuResource(), positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetDefaultState(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE );
		}
		if( positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetGpuResource() != indices.m_indexBuffer.TrinityALImpl_GetObject()->GetGpuResource() &&
			( indices.m_indexBuffer.TrinityALImpl_GetObject()->GetDefaultState() & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE ) == 0 )
		{
			barriers[barrierCount++] = TrinityALImpl::Transition( indices.m_indexBuffer.TrinityALImpl_GetObject()->GetGpuResource(), indices.m_indexBuffer.TrinityALImpl_GetObject()->GetDefaultState(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE );
		}
		if( barrierCount )
		{
			renderContext.ResourceBarrierDx12( barrierCount, barriers );
		}

		renderContext.FlushBarriersDx12( positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetGpuResource(), indices.m_indexBuffer.TrinityALImpl_GetObject()->GetGpuResource() );

		renderContext.m_commandList4->BuildRaytracingAccelerationStructure( &desc, 0, nullptr );

		std::swap( barriers[0].Transition.StateAfter, barriers[0].Transition.StateBefore );
		std::swap( barriers[1].Transition.StateAfter, barriers[1].Transition.StateBefore );
		renderContext.ResourceBarrierDx12( barrierCount, barriers );

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
			m_owner = nullptr;
			m_scratch = nullptr;
			m_buffer = nullptr;
			m_bufferAddress = {};
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
