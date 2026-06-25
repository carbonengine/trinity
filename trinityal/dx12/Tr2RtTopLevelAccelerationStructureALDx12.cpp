// Copyright © 2019 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2RtTopLevelAccelerationStructureALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "Tr2BufferALDx12.h"
#include "Tr2RtBottomLevelAccelerationStructureALDx12.h"
#include "Utilities.h"


namespace
{
uintptr_t Align( uintptr_t offset, size_t alignment )
{
	return ( offset + ( alignment - 1 ) ) & ~( alignment - 1 );
}

ALResult UploadInstanceData( ID3D12Resource* uploadBuffer, const size_t count, const Tr2RtInstanceAL* instances )
{
	D3D12_RAYTRACING_INSTANCE_DESC* data;
	CR_RETURN_HR( uploadBuffer->Map( 0, nullptr, reinterpret_cast<void**>( &data ) ) );
	for( size_t i = 0; i < count; ++i )
	{
		if( !instances[i].blas->IsValid() )
		{
			uploadBuffer->Unmap( 0, nullptr );
			return E_INVALIDARG;
		}
		D3D12_RAYTRACING_INSTANCE_DESC d;
		d.InstanceID = i;
		d.InstanceContributionToHitGroupIndex = instances[i].materialIndex;
		d.InstanceMask = 0xff;
		memcpy( d.Transform, instances[i].transform, 12 * sizeof( float ) );
		d.AccelerationStructure = instances[i].blas->GetGPUVirtualAddress();
		d.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE | instances[i].flags;
		memcpy( data + i, &d, sizeof( d ) );
	}
	uploadBuffer->Unmap( 0, nullptr );
	return S_OK;
}
}


namespace TrinityALImpl
{
Tr2RtTopLevelAccelerationStructureAL::Tr2RtTopLevelAccelerationStructureAL() :
	m_capacity( 0 ),
	m_buildFlags( Tr2RtBuildFlags::NONE ),
	m_owner( nullptr )
{
}

Tr2RtTopLevelAccelerationStructureAL::~Tr2RtTopLevelAccelerationStructureAL()
{
	Destroy();
}

ALResult Tr2RtTopLevelAccelerationStructureAL::Create( const size_t count, const Tr2RtInstanceAL* instances, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext )
{
	if( !renderContext.IsValid() || !renderContext.GetCaps().SupportsRaytracing() )
	{
		return E_INVALIDCALL;
	}
	if( count == 0 )
	{
		return E_INVALIDARG;
	}
	if( !renderContext.m_commandList4 )
	{
		return E_INVALIDCALL;
	}

	//if this causes problems then create an array of all the instances.blas buffers fill the uav barrier with that
	D3D12_RESOURCE_BARRIER uavBarrier;
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = nullptr;
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	renderContext.m_commandList->ResourceBarrier( 1, &uavBarrier );

	size_t capacity = Align( count, 128 );

	auto uploadHeap = TrinityALImpl::HeapDesc( D3D12_HEAP_TYPE_UPLOAD );
	auto uploadDesc = TrinityALImpl::BufferDesc( sizeof( D3D12_RAYTRACING_INSTANCE_DESC ) * capacity, D3D12_RESOURCE_FLAG_NONE );
	CComPtr<ID3D12Resource> uploadBuffer;
	CR_RETURN_HR( renderContext.m_device->CreateCommittedResource(
		&uploadHeap,
		D3D12_HEAP_FLAG_NONE,
		&uploadDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS( &uploadBuffer ) ) );

	CR_RETURN_HR( UploadInstanceData( uploadBuffer, count, instances ) );

	// Describe the work being requested
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS ASInputs = {};
	ASInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	ASInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	ASInputs.InstanceDescs = uploadBuffer->GetGPUVirtualAddress();
	ASInputs.NumDescs = UINT( capacity );
	ASInputs.Flags = (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS)buildFlags;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO ASBuildInfo = {};
	renderContext.m_device5->GetRaytracingAccelerationStructurePrebuildInfo( &ASInputs, &ASBuildInfo );

	if( ASBuildInfo.ResultDataMaxSizeInBytes <= 0 )
	{
		return E_INVALIDCALL;
	}
	ASInputs.NumDescs = UINT( count );

	CComPtr<ID3D12Resource> scratch;
	auto heap = TrinityALImpl::HeapDesc( D3D12_HEAP_TYPE_DEFAULT );
	auto scratchDesc = TrinityALImpl::BufferDesc(
		Align( ASBuildInfo.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT ),
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS );
	CR_RETURN_HR( renderContext.m_device->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&scratchDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS( &scratch ) ) );

	::Tr2BufferAL buffer;
	CR_RETURN_HR( buffer.Create(
		Tr2BufferDescriptionAL( 1,
								uint32_t( Align( ASBuildInfo.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT ) ),
								Tr2GpuUsage::ACCELERATION_STRUCTURE | Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::UNORDERED_ACCESS,
								Tr2CpuUsage::NONE ),
		nullptr,
		renderContext ) );

	if( !buffer.IsValid() )
	{
		return E_INVALIDCALL;
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
	desc.Inputs = ASInputs;

	desc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
	desc.DestAccelerationStructureData = buffer.TrinityALImpl_GetObject()->GetGpuView();

	if( renderContext.m_commandList4 )
	{
		renderContext.m_commandList4->BuildRaytracingAccelerationStructure( &desc, 0, nullptr );
	}

	// don't compact things if they animate
	// but here we would maybe compact

	m_buffer = buffer;
	m_scratch = scratch;
	UploadBuffer ub = { uploadBuffer, renderContext.GetRecordingFrameNumber() };
	m_capacity = capacity;
	m_buildFlags = buildFlags;
	m_owner = &renderContext;
	m_uploadBuffers.push_back( ub );

	D3D12_RESOURCE_BARRIER topLevelUavBarrier;
	topLevelUavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	topLevelUavBarrier.UAV.pResource = buffer.TrinityALImpl_GetObject()->GetGpuResource();
	topLevelUavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	renderContext.m_commandList->ResourceBarrier( 1, &topLevelUavBarrier );

	return S_OK;
}

// DXR suggests to rebuild the TLAS every frame instead of updating it
ALResult Tr2RtTopLevelAccelerationStructureAL::Update( const size_t count, const Tr2RtInstanceAL* instances, Tr2RenderContextAL& renderContext )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
	if( m_capacity < count )
	{
		return E_INVALIDARG;
	}
	if( !renderContext.m_commandList4 )
	{
		return E_INVALIDCALL;
	}

	//if this causes problems then create an array of all the instances.blas buffers fill the uav barrier with that
	D3D12_RESOURCE_BARRIER uavBarrier;
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = nullptr;
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	renderContext.m_commandList->ResourceBarrier( 1, &uavBarrier );

	CComPtr<ID3D12Resource> uploadBuffer;
	auto completed = m_owner->GetRenderedFrameNumber();
	for( auto it = begin( m_uploadBuffers ); it != end( m_uploadBuffers ); ++it )
	{
		if( completed >= it->frameIndex )
		{
			uploadBuffer = it->uploadBuffer;
			it->frameIndex = m_owner->GetRecordingFrameNumber();
			break;
		}
	}
	if( !uploadBuffer )
	{
		auto uploadHeap = TrinityALImpl::HeapDesc( D3D12_HEAP_TYPE_UPLOAD );
		auto uploadDesc = TrinityALImpl::BufferDesc( sizeof( D3D12_RAYTRACING_INSTANCE_DESC ) * m_capacity, D3D12_RESOURCE_FLAG_NONE );
		CR_RETURN_HR( m_owner->m_device->CreateCommittedResource(
			&uploadHeap,
			D3D12_HEAP_FLAG_NONE,
			&uploadDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS( &uploadBuffer ) ) );

		UploadBuffer ub = { uploadBuffer, m_owner->GetRecordingFrameNumber() };
		m_uploadBuffers.push_back( ub );
	}

	CR_RETURN_HR( UploadInstanceData( uploadBuffer, count, instances ) );

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS ASInputs = {};
	ASInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	ASInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	ASInputs.InstanceDescs = uploadBuffer->GetGPUVirtualAddress();
	ASInputs.NumDescs = UINT( count );
	ASInputs.Flags = (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS)m_buildFlags;
	if( HasFlag( m_buildFlags, Tr2RtBuildFlags::ALLOW_UPDATE ) )
	{
		ASInputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
	desc.Inputs = ASInputs;

	if( HasFlag( m_buildFlags, Tr2RtBuildFlags::ALLOW_UPDATE ) )
	{
		desc.SourceAccelerationStructureData = m_buffer.TrinityALImpl_GetObject()->GetGpuView();
	}
	desc.ScratchAccelerationStructureData = m_scratch->GetGPUVirtualAddress();
	desc.DestAccelerationStructureData = m_buffer.TrinityALImpl_GetObject()->GetGpuView();

	if( renderContext.m_commandList4 )
	{
		renderContext.m_commandList4->BuildRaytracingAccelerationStructure( &desc, 0, nullptr );
	}

	D3D12_RESOURCE_BARRIER topLevelUavBarrier;
	topLevelUavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	topLevelUavBarrier.UAV.pResource = m_buffer.TrinityALImpl_GetObject()->GetGpuResource();
	topLevelUavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	renderContext.m_commandList->ResourceBarrier( 1, &topLevelUavBarrier );

	return S_OK;
}

bool Tr2RtTopLevelAccelerationStructureAL::IsValid() const
{
	return m_owner != nullptr;
}

void Tr2RtTopLevelAccelerationStructureAL::Destroy()
{
	if( m_owner )
	{
		for( auto it = begin( m_uploadBuffers ); it != end( m_uploadBuffers ); ++it )
		{
			RELEASE_LATER( m_owner, it->uploadBuffer );
		}
		m_uploadBuffers.clear();
		if( m_scratch )
		{
			RELEASE_LATER( m_owner, m_scratch );
		}
		m_scratch = nullptr;
		m_capacity = 0;
		m_buildFlags = Tr2RtBuildFlags::NONE;
		m_owner = nullptr;
	}
}

Tr2ALMemoryType Tr2RtTopLevelAccelerationStructureAL::GetMemoryClass() const
{
	return AL_MEMORY_MANAGED;
}

void Tr2RtTopLevelAccelerationStructureAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2RtTopLevelAccelerationStructureAL";
}


const ::Tr2BufferAL& Tr2RtTopLevelAccelerationStructureAL::GetBuffer() const
{
	return m_buffer;
}

size_t Tr2RtTopLevelAccelerationStructureAL::GetCapacity() const
{
	return m_capacity;
}
}

#endif
