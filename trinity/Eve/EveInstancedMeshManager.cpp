// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveInstancedMeshManager.h"
#include "../Tr2GpuStructuredBuffer.h"


CCP_STATS_DECLARE( eimmPerInstanceSize, "Trinity/EveInstancedMeshManager/perInstanceSize", true, CST_MEMORY, "Total size of per-object data uploaded per frame" );
CCP_STATS_DECLARE( eimmPerInstanceBufferSize, "Trinity/EveInstancedMeshManager/perInstanceBufferSize", true, CST_MEMORY, "Size of the per-object data buffer" );
CCP_STATS_DECLARE( eimmInstanceCount, "Trinity/EveInstancedMeshManager/instanceCount", true, CST_COUNTER_HIGH, "Total number of instances" );
CCP_STATS_DECLARE( eimmMeshCount, "Trinity/EveInstancedMeshManager/meshCount", true, CST_COUNTER_HIGH, "Total number of meshes" );
CCP_STATS_DECLARE( eimmBatchesCount, "Trinity/EveInstancedMeshManager/batchCount", true, CST_COUNTER_HIGH, "Total number of batches collected" );
CCP_STATS_DECLARE( eimmInstancesRendered, "Trinity/EveInstancedMeshManager/instancesRendered", true, CST_COUNTER_HIGH, "Total number of instances rendered" );
CCP_STATS_DECLARE( eimmStaticInstanceBufferSize, "Trinity/EveInstancedMeshManager/staticInstanceBufferSize", true, CST_MEMORY, "Size of the static per-instance data buffer" );
CCP_STATS_DECLARE( eimmDynamicInstanceBufferSize, "Trinity/EveInstancedMeshManager/dynamicInstanceBufferSize", true, CST_MEMORY, "Size of the dynamic per-instance data buffer" );


namespace
{
constexpr float SCREEN_SIZE_THRESHOLD = 5.0f;
constexpr float SHADOW_SIZE_THRESHOLD = 5.0f;

struct PickingPerObjectData : public Tr2PerObjectData
{
	void ApplyConstantBuffers( Tr2IndirectDrawBufferWriter& writer, Tr2RenderContext& renderContext ) const override
	{
	}
};

template <typename T>
ITriRenderBatchAccumulator* FindBatchAccumulator( const T& batches, TriBatchType batchType )
{
	for( auto& pair : batches )
	{
		if( pair.first == batchType )
		{
			return &pair.second;
		}
	}
	return nullptr;
}
}


void EveInstancedMeshManager::CollectMeshes( EveComponentRegistry& registry )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	USE_MAIN_THREAD_RENDER_CONTEXT();
	auto renderedFrame = renderContext.GetRenderedFrameNumber();
	m_staticInstanceBuffer.TrimRegions( renderedFrame );
	m_dynamicInstanceBuffer.TrimRegions( renderedFrame );

	registry.ProcessComponents<IEveInstanceMeshProvider>( [&]( IEveInstanceMeshProvider* meshes ) {
		meshes->AddMeshesToManager( *this );
	} );


	if( !m_perObjectData.empty() )
	{
		if( !m_perObjectDataBuffer )
		{
			m_perObjectDataBuffer.CreateInstance();
		}
		auto count = static_cast<uint32_t>( m_perObjectData.size() );
		if( m_perObjectDataBuffer->GetCount() < count )
		{
			auto newCount = std::max( count, m_perObjectDataBuffer->GetCount() * 2u );
			m_perObjectDataBuffer->Create( newCount, sizeof( EveSpacePerObjectData ), Tr2GpuStructuredBuffer::CPU_WRITABLE );
		}

		uint8_t* mappedData = nullptr;
		m_perObjectDataBuffer->GetGpuBuffer( 0 )->MapForWriting( mappedData, renderContext );
		for( auto& pod : m_perObjectData )
		{
			memcpy( mappedData, pod.first, sizeof( EveSpacePerObjectData ) );
			mappedData += sizeof( EveSpacePerObjectData );
		}
		m_perObjectDataBuffer->GetGpuBuffer( 0 )->UnmapForWriting( renderContext );

		CCP_STATS_ADD( eimmPerInstanceSize, sizeof( EveSpacePerObjectData ) * m_perObjectData.size() );
	}

	if( m_perObjectDataBuffer )
	{
		CCP_STATS_ADD( eimmPerInstanceBufferSize, sizeof( EveSpacePerObjectData ) * m_perObjectDataBuffer->GetCount() );
	}
	CCP_STATS_ADD( eimmInstanceCount, m_instanceCount );
	CCP_STATS_ADD( eimmMeshCount, m_meshInstances.size() );

	GlobalStore().RegisterVariable( "InstancedPerObjectBuffer", m_perObjectDataBuffer );

	CCP_STATS_ADD( eimmStaticInstanceBufferSize, m_staticInstanceBuffer.GetSize() );
	CCP_STATS_ADD( eimmDynamicInstanceBufferSize, m_dynamicInstanceBuffer.GetSize() );
}

void EveInstancedMeshManager::AddPerObjectData( PerObjectDataHandle& handle, const EveSpacePerObjectData* data )
{
	CCP_ASSERT( !handle );
	if( !handle )
	{
		uint32_t offset = static_cast<uint32_t>( m_perObjectData.size() );
		m_perObjectData.push_back( { data, &handle } );
		handle.index = offset;
		handle.owner = this;
	}
}

void EveInstancedMeshManager::RemovePerObjectData( PerObjectDataHandle& handle )
{
	auto oldPerObjectDataIndex = m_perObjectData.back().second->index;
	auto newPerObjectDataIndex = handle.index;
	if( oldPerObjectDataIndex != newPerObjectDataIndex )
	{
		for( auto& [key, mesh] : m_meshInstances )
		{
			for( auto& group : mesh.meshGroups )
			{
				if( group.perObjectDataIndex == oldPerObjectDataIndex )
				{
					group.perObjectDataIndex = newPerObjectDataIndex;
				}
			}
		}
	}

	std::swap( m_perObjectData[handle.index], m_perObjectData.back() );
	m_perObjectData[handle.index].second->index = handle.index;
	m_perObjectData.pop_back();

	handle.index = PerObjectDataHandle::InvalidIndex;
}

void EveInstancedMeshManager::ReplaceHandle( PerObjectDataHandle* oldHandle, PerObjectDataHandle* newHandle )
{
	if( *oldHandle )
	{
		auto& group = m_perObjectData[oldHandle->index];
		group.second = newHandle;
	}
}

void EveInstancedMeshManager::AddBoundingSphereGroup( BoundingSphereHandle& handle, const CcpMath::Sphere& bounds, const InstanceFlags& flags, const CcpMath::Sphere* boundingSpheres, uint32_t count )
{
	CCP_ASSERT( !handle );
	if( !handle )
	{
		auto& group = m_sphereGroups.emplace_back();
		group.bounds = bounds;
		group.flags = flags;
		group.count = count;
		group.boundingSpheres = boundingSpheres;
		group.handle = &handle;
		group.screenSizes.resize( count, 0.0f );
		handle.owner = this;
		handle.index = static_cast<uint32_t>( m_sphereGroups.size() - 1 );
	}
}

void EveInstancedMeshManager::RemoveBoundingSphereGroup( BoundingSphereHandle& handle )
{
	CCP_ASSERT( handle );
	if( handle )
	{
		auto oldSphereGroupIndex = m_sphereGroups.back().handle->index;
		auto newSphereGroupIndex = handle.index;
		if( oldSphereGroupIndex != newSphereGroupIndex )
		{
			for( auto& [key, mesh] : m_meshInstances )
			{
				for( auto& group : mesh.meshGroups )
				{
					if( group.sphereGroupIndex == oldSphereGroupIndex )
					{
						group.sphereGroupIndex = newSphereGroupIndex;
					}
				}
			}
		}

		std::swap( m_sphereGroups[handle.index], m_sphereGroups.back() );
		m_sphereGroups[handle.index].handle->index = handle.index;
		m_sphereGroups.pop_back();
		handle.index = BoundingSphereHandle::InvalidIndex;
		handle.owner = nullptr;
	}
}

void EveInstancedMeshManager::SetSphereGroupBounds( const BoundingSphereHandle& handle, const CcpMath::Sphere& bounds, const InstanceFlags& flags )
{
	auto& group = m_sphereGroups[handle.index];
	group.bounds = bounds;
	group.flags = flags;
}

void EveInstancedMeshManager::ReplaceHandle( BoundingSphereHandle* oldHandle, BoundingSphereHandle* newHandle )
{
	if( *oldHandle )
	{
		auto& group = m_sphereGroups[oldHandle->index];
		group.handle = newHandle;
	}
}

void EveInstancedMeshManager::AddMeshGroup(
	MeshGroupHandle& handle,
	TriGeometryRes* geometry,
	unsigned combinedVertexDeclaration,
	TriBatchType batchType,
	uint32_t meshIndex,
	uint32_t areaIndex,
	uint32_t areaCount,
	Tr2Effect* material,
	uint64_t materialHash,
	const PerObjectDataHandle& perObjectDataHandle,
	const BoundingSphereHandle& sphereHandle,
	const DynamicPerInstanceData* perInstanceData,
	uint32_t count,
	IRoot* pickingOwner,
	uint32_t pickingOwnerIndex )
{
	CCP_ASSERT( !handle );
	CCP_ASSERT( perObjectDataHandle );
	CCP_ASSERT( sphereHandle );

	MeshKey key = { geometry,
					materialHash,
					combinedVertexDeclaration,
					batchType,
					meshIndex,
					areaIndex,
					areaCount,
					true };
	auto& instances = m_meshInstances[key];
	instances.material = material;
	if( instances.radius == 0 )
	{
		auto meshData = geometry->GetMeshData( meshIndex );
		auto sphere = CcpMath::Sphere( CcpMath::AxisAlignedBox( meshData->m_minBounds, meshData->m_maxBounds ) );
		instances.radius = sphere.radius + Length( sphere.center );
	}
	if( instances.lodIndices.empty() )
	{
		auto meshData = geometry->GetMeshData( meshIndex );
		auto lodCount = meshData->m_lods.size();
		instances.lodIndices.resize( lodCount );
		instances.screenSizeThresholds.resize( lodCount );
		for( uint32_t lod = 0; lod < lodCount; ++lod )
		{
			instances.screenSizeThresholds[lod] = meshData->m_lods[lod]->m_maxScreenSize;
		}
	}

	auto& meshGroup = instances.meshGroups.emplace_back();
	meshGroup.perObjectDataIndex = perObjectDataHandle.index;
	meshGroup.sphereGroupIndex = sphereHandle.index;
	meshGroup.dynamicInstances = perInstanceData;
	meshGroup.count = count;
	meshGroup.handle = &handle;
	meshGroup.owner = pickingOwner;
	meshGroup.ownerIndex = pickingOwnerIndex;
	handle.owner = this;
	handle.index = static_cast<uint32_t>( instances.meshGroups.size() - 1 );

	m_instanceCount += count;
}

void EveInstancedMeshManager::AddMeshGroup(
	MeshGroupHandle& handle,
	TriGeometryRes* geometry,
	unsigned combinedVertexDeclaration,
	TriBatchType batchType,
	uint32_t meshIndex,
	uint32_t areaIndex,
	uint32_t areaCount,
	Tr2Effect* material,
	uint64_t materialHash,
	const PerObjectDataHandle& perObjectDataHandle,
	const BoundingSphereHandle& sphereHandle,
	const StaticPerInstanceData* perInstanceData,
	uint32_t count,
	IRoot* pickingOwner,
	uint32_t pickingOwnerIndex )
{
	CCP_ASSERT( !handle );
	CCP_ASSERT( perObjectDataHandle );
	CCP_ASSERT( sphereHandle );

	MeshKey key = { geometry,
					materialHash,
					combinedVertexDeclaration,
					batchType,
					meshIndex,
					areaIndex,
					areaCount };
	auto& instances = m_meshInstances[key];
	instances.material = material;
	if( instances.radius == 0 )
	{
		auto meshData = geometry->GetMeshData( meshIndex );
		auto sphere = CcpMath::Sphere( CcpMath::AxisAlignedBox( meshData->m_minBounds, meshData->m_maxBounds ) );
		instances.radius = sphere.radius + Length( sphere.center );
	}
	if( instances.lodIndices.empty() )
	{
		auto meshData = geometry->GetMeshData( meshIndex );
		auto lodCount = meshData->m_lods.size();
		instances.lodIndices.resize( lodCount );
		instances.screenSizeThresholds.resize( lodCount );
		for( uint32_t lod = 0; lod < lodCount; ++lod )
		{
			instances.screenSizeThresholds[lod] = meshData->m_lods[lod]->m_maxScreenSize;
		}
	}

	auto& meshGroup = instances.meshGroups.emplace_back();
	meshGroup.perObjectDataIndex = perObjectDataHandle.index;
	meshGroup.sphereGroupIndex = sphereHandle.index;
	meshGroup.staticInstances = perInstanceData;
	meshGroup.count = count;
	meshGroup.handle = &handle;
	meshGroup.owner = pickingOwner;
	meshGroup.ownerIndex = pickingOwnerIndex;
	handle.owner = this;
	handle.index = static_cast<uint32_t>( instances.meshGroups.size() - 1 );

	m_instanceCount += count;
}

void EveInstancedMeshManager::RemoveMeshGroup( MeshGroupHandle& handle )
{
	CCP_ASSERT( handle );
	if( handle )
	{
		for( auto& [key, mesh] : m_meshInstances )
		{
			if( mesh.meshGroups.size() > handle.index && mesh.meshGroups[handle.index].handle == &handle )
			{
				m_instanceCount -= mesh.meshGroups[handle.index].count;

				std::swap( mesh.meshGroups[handle.index], mesh.meshGroups.back() );
				mesh.meshGroups[handle.index].handle->index = handle.index;
				mesh.meshGroups.pop_back();

				if( mesh.meshGroups.empty() )
				{
					m_meshInstances.erase( key );
				}
				break;
			}
		}
		handle.index = MeshGroupHandle::InvalidIndex;
	}
}

void EveInstancedMeshManager::ReplaceHandle( MeshGroupHandle* oldHandle, MeshGroupHandle* newHandle )
{
	for( auto& [key, mesh] : m_meshInstances )
	{
		if( mesh.meshGroups.size() > newHandle->index && mesh.meshGroups[newHandle->index].handle == oldHandle )
		{
			mesh.meshGroups[newHandle->index].handle = newHandle;
			break;
		}
	}
}


EveInstancedMeshManager::InstanceBuffer::Allocation EveInstancedMeshManager::InstanceBuffer::Allocate( uint32_t count )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	auto frameNumber = renderContext.GetRecordingFrameNumber();

	auto offset = GetUnusedRegion( count );
	if( !offset.has_value() )
	{
		if( mappedData )
		{
			mappedRetiredBuffers.push_back( buffer.get() );
			mappedData = nullptr;
		}
		auto oldCount = buffer ? buffer->GetDesc().count : 0;
		retiredBuffers.push_back( std::move( buffer ) );
		buffer = std::make_unique<Tr2BufferAL>();
		uint32_t newCount = std::max( count, oldCount ? oldCount * 2 : 128 * 1024 );
		buffer->Create( 4, newCount, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::WRITE_OFTEN | Tr2CpuUsage::NON_SYNCRONIZED_WRITE, nullptr, renderContext );
		regions.clear();
		offset = 0;
	}

	buffer->MapForWriting( mappedData, renderContext );

	if( !regions.empty() && regions.back().recordedFrame == frameNumber && regions.back().offset + regions.back().length == offset )
	{
		regions.back().length += count;
	}
	else
	{
		regions.push_back( { offset.value(), count, frameNumber } );
	}
	return { buffer.get(), mappedData + offset.value(), offset.value() };
}

std::optional<uint32_t> EveInstancedMeshManager::InstanceBuffer::GetUnusedRegion( uint32_t count )
{
	if( !buffer || !buffer->IsValid() )
	{
		return {};
	}
	uint32_t totalCount = buffer->GetDesc().count * buffer->GetDesc().stride;

	if( regions.empty() )
	{
		if( count <= totalCount )
		{
			return 0;
		}
		return {};
	}

	auto offset = regions.back().offset + regions.back().length;
	uint32_t endOffset = regions.front().offset;
	if( endOffset < offset )
	{
		if( count < totalCount - offset )
		{
			return offset;
		}
		if( count < endOffset )
		{
			offset = 0;
			return offset;
		}
		return {};
	}
	if( endOffset >= offset + count )
	{
		return offset;
	}
	return {};
}

void EveInstancedMeshManager::InstanceBuffer::DoneCopying()
{
	if( mappedData )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		buffer->UnmapForWriting( renderContext );
		mappedData = nullptr;
	}
	for( auto& buf : mappedRetiredBuffers )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		buf->UnmapForWriting( renderContext );
	}
	mappedRetiredBuffers.clear();
}

void EveInstancedMeshManager::InstanceBuffer::TrimRegions( uint64_t renderedFrame )
{
	auto used = regions.begin();
	while( used != regions.end() )
	{
		if( used->recordedFrame > renderedFrame )
		{
			break;
		}
		++used;
	}
	regions.erase( regions.begin(), used );
	retiredBuffers.clear();
}

uint32_t EveInstancedMeshManager::InstanceBuffer::GetSize() const
{
	return buffer ? buffer->GetDesc().count * buffer->GetDesc().stride : 0;
}

void EveInstancedMeshManager::PerformFrustumCulling( const TriFrustum& cameraFrustum, float invLodFactor, InstanceFlags filter )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Tr2ParallelDo(
		begin( m_sphereGroups ),
		end( m_sphereGroups ),
		[&]( auto& group ) {
			if( group.flags.MatchesFilter( filter ) )
			{
				auto res = cameraFrustum.SphereTest( group.bounds );
				switch( res )
				{
				case TriFrustumTestResult::Outside:
					break;
				case TriFrustumTestResult::Inside:
					for( uint32_t i = 0; i < group.screenSizes.size(); ++i )
					{
						const auto& sphere = group.boundingSpheres[i];
						float size = cameraFrustum.GetPixelSizeAccrossEst( sphere.center, sphere.radius ) * invLodFactor;
						group.screenSizes[i] = size;
					}
					break;
				default:
					for( uint32_t i = 0; i < group.screenSizes.size(); ++i )
					{
						const auto& sphere = group.boundingSpheres[i];
						float size = 0;
						if( cameraFrustum.IsSphereVisible( sphere.center, sphere.radius ) )
						{
							size = cameraFrustum.GetPixelSizeAccrossEst( sphere.center, sphere.radius ) * invLodFactor;
						}
						group.screenSizes[i] = size;
					}
					break;
				}
				group.lastTestResult = res;
			}
			else
			{
				group.lastTestResult = TriFrustumTestResult::Outside;
			}
		} );
}

void EveInstancedMeshManager::PerformFrustumCulling( const TriFrustum& cameraFrustum, const TriFrustum& pickingFrustum, float invLodFactor, InstanceFlags filter )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Tr2ParallelDo(
		begin( m_sphereGroups ),
		end( m_sphereGroups ),
		[&]( auto& group ) {
			if( group.flags.MatchesFilter( filter ) )
			{
				auto res = pickingFrustum.SphereTest( group.bounds );
				switch( res )
				{
				case TriFrustumTestResult::Outside:
					break;
				case TriFrustumTestResult::Inside:
					for( uint32_t i = 0; i < group.screenSizes.size(); ++i )
					{
						const auto& sphere = group.boundingSpheres[i];
						float size = cameraFrustum.GetPixelSizeAccrossEst( sphere.center, sphere.radius ) * invLodFactor;
						group.screenSizes[i] = size;
					}
					break;
				default:
					for( uint32_t i = 0; i < group.screenSizes.size(); ++i )
					{
						const auto& sphere = group.boundingSpheres[i];
						float size = 0;
						if( pickingFrustum.IsSphereVisible( sphere.center, sphere.radius ) )
						{
							size = cameraFrustum.GetPixelSizeAccrossEst( sphere.center, sphere.radius ) * invLodFactor;
						}
						group.screenSizes[i] = size;
					}
					break;
				}
				group.lastTestResult = res;
			}
			else
			{
				group.lastTestResult = TriFrustumTestResult::Outside;
			}
		} );
}

void EveInstancedMeshManager::PerformFrustumCulling( const TriFrustum& cameraFrustum, const IEveShadowFrustum& shadowFrustum, float invLodFactor, InstanceFlags filter )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	tbb::parallel_for_each(
		begin( m_sphereGroups ),
		end( m_sphereGroups ),
		[&]( auto& group ) {
			if( group.flags.MatchesFilter( filter ) )
			{
				auto res = shadowFrustum.SphereTest( cameraFrustum, group.bounds );
				switch( res )
				{
				case TriFrustumTestResult::Outside:
					break;
				case TriFrustumTestResult::Inside:
					for( uint32_t i = 0; i < group.screenSizes.size(); ++i )
					{
						const auto& sphere = group.boundingSpheres[i];
						float size = 0;
						if( shadowFrustum.GetSizeInShadow( Vector4( sphere.center, sphere.radius ) ) >= SHADOW_SIZE_THRESHOLD )
						{
							size = cameraFrustum.GetPixelSizeAccrossEst( sphere.center, sphere.radius ) * invLodFactor;
						}
						group.screenSizes[i] = size;
					}
					break;
				default:
					for( uint32_t i = 0; i < group.screenSizes.size(); ++i )
					{
						const auto& sphere = group.boundingSpheres[i];
						float size = 0;
						if( shadowFrustum.IsVisible( cameraFrustum, Vector4( sphere.center, sphere.radius ) ) )
						{
							if( shadowFrustum.GetSizeInShadow( Vector4( sphere.center, sphere.radius ) ) >= SHADOW_SIZE_THRESHOLD )
							{
								size = cameraFrustum.GetPixelSizeAccrossEst( sphere.center, sphere.radius ) * invLodFactor;
							}
						}
						group.screenSizes[i] = size;
					}
					break;
				}
				group.lastTestResult = res;
			}
			else
			{
				group.lastTestResult = TriFrustumTestResult::Outside;
			}
		} );
}

std::pair<uint32_t, float> EveInstancedMeshManager::BinVisibleInstances( const MeshKey& mesh, MeshData& meshInfo )
{
	for( auto& lodIndices : meshInfo.lodIndices )
	{
		lodIndices.clear();
	}

	uint32_t totalCount = 0;
	float maxScreenSize = 0.0f;

	for( auto& group : meshInfo.meshGroups )
	{
		auto [groupCount, groupMaxSize] = BinVisibleInstances( mesh, meshInfo, group );
		totalCount += groupCount;
		maxScreenSize = std::max( maxScreenSize, groupMaxSize );
	}
	return { totalCount, maxScreenSize };
}

std::pair<uint32_t, float> EveInstancedMeshManager::BinVisibleInstances( const MeshKey& mesh, MeshData& meshInfo, MeshGroup& group )
{
	uint32_t totalCount = 0;
	float maxScreenSize = 0.0f;

	CCP_ASSERT( group.sphereGroupIndex < m_sphereGroups.size() );
	auto& sphereGroup = m_sphereGroups[group.sphereGroupIndex];
	if( sphereGroup.lastTestResult != TriFrustumTestResult::Outside )
	{
		for( uint32_t i = 0; i < group.count; ++i )
		{
			float screenSize = sphereGroup.screenSizes[mesh.isDynamic ? group.dynamicInstances[i].sphereIndex : group.staticInstances[i].sphereIndex];
			maxScreenSize = std::max( maxScreenSize, screenSize );
			if( screenSize > SCREEN_SIZE_THRESHOLD )
			{
				meshInfo.lodIndices[GetMeshLod( meshInfo, screenSize )].push_back( { mesh.isDynamic ? (const void*)&group.dynamicInstances[i] : (const void*)&group.staticInstances[i], group.perObjectDataIndex } );
				++totalCount;
			}
		}
	}
	return { totalCount, maxScreenSize };
}

size_t EveInstancedMeshManager::GetBatches( const TriFrustum& cameraFrustum, float invLodFactor, const std::initializer_list<std::pair<TriBatchType, ITriRenderBatchAccumulator&>>& batches, Tr2RenderReason reason )
{
	InstanceFlags filter;
	for( auto& pair : batches )
	{
		filter.AddBatchType( pair.first );
	}
	if( reason == TR2RENDERREASON_REFLECTION )
	{
		filter.SetRenderInReflections( true );
	}
	PerformFrustumCulling( cameraFrustum, invLodFactor, filter );

	return GetBatches( batches );
}

size_t EveInstancedMeshManager::GetShadowBatches( const TriFrustum& cameraFrustum, const IEveShadowFrustum& shadowFrustum, float invLodFactor, const std::initializer_list<std::pair<TriBatchType, ITriRenderBatchAccumulator&>>& batches, Tr2RenderReason reason )
{
	InstanceFlags filter;
	for( auto& pair : batches )
	{
		filter.AddBatchType( pair.first );
	}
	if( reason == TR2RENDERREASON_REFLECTION )
	{
		filter.SetRenderInReflections( true );
	}
	filter.SetCastsShadow( true );
	PerformFrustumCulling( cameraFrustum, shadowFrustum, invLodFactor, filter );

	return GetBatches( batches );
}

void EveInstancedMeshManager::GetPickingBatches( EvePendingPickingReadback& readback, const TriFrustum& viewFrustum, const TriFrustum& pickingFrustum, float invLodFactor, uint32_t objectIdOffset, const std::vector<std::pair<TriBatchType, ITriRenderBatchAccumulator&>>& batches )
{
	InstanceFlags filter;
	for( auto& pair : batches )
	{
		filter.AddBatchType( pair.first );
	}
	PerformFrustumCulling( viewFrustum, pickingFrustum, invLodFactor, filter );

	GetPickingBatches( readback, objectIdOffset, batches );
}

std::pair<IRootPtr, uint32_t> EveInstancedMeshManager::GetPickedObject( uint32_t objectId, uint32_t areaId )
{
	for( auto& [mesh, meshInfo] : m_meshInstances )
	{
		for( auto& group : meshInfo.meshGroups )
		{
			if( group.pickingObjectId == 0xffffffff )
			{
				continue;
			}
			if( group.pickingObjectId > objectId || group.pickingObjectId + meshInfo.lodIndices.size() <= objectId )
			{
				continue;
			}
			if( m_sphereGroups[group.sphereGroupIndex].lastTestResult == TriFrustumTestResult::Outside )
			{
				continue;
			}
			BinVisibleInstances( mesh, meshInfo, group );
			auto& lod = meshInfo.lodIndices[objectId - group.pickingObjectId];
			uint32_t instanceId = 0;
			if( areaId < lod.size() )
			{
				instanceId = uint32_t( mesh.isDynamic ? static_cast<const DynamicPerInstanceData*>( lod[areaId].first ) - group.dynamicInstances : static_cast<const StaticPerInstanceData*>( lod[areaId].first ) - group.staticInstances );
			}
			return { group.owner, instanceId | ( group.ownerIndex << 16 ) };
		}
	}
	return { nullptr, 0 };
}


void EveInstancedMeshManager::BinVisibleInstances( const std::initializer_list<std::pair<TriBatchType, ITriRenderBatchAccumulator&>>& batches )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	tbb::parallel_for_each(
		begin( m_meshInstances ),
		end( m_meshInstances ),
		[&]( auto& kv ) {
			auto& mesh = kv.first;
			auto& meshInfo = kv.second;
			if( !FindBatchAccumulator( batches, mesh.batchType ) )
			{
				meshInfo.totalVisibleInstances = 0;
				return;
			}

			auto [instanceCount, maxScreenSize] = BinVisibleInstances( mesh, meshInfo );
			meshInfo.maxScreenSize = maxScreenSize;
			meshInfo.totalVisibleInstances = instanceCount;
		} );
}


void EveInstancedMeshManager::GetPickingBatches( EvePendingPickingReadback& readback, uint32_t objectIdOffset, const std::vector<std::pair<TriBatchType, ITriRenderBatchAccumulator&>>& batches )
{
	CCP_STATS_ZONE( __FUNCTION__ );


	std::vector<std::pair<IRootPtr, uint32_t>>& traceback = readback.m_instancedTraceback;

	for( auto& [mesh, meshInfo] : m_meshInstances )
	{
		auto accumulator = FindBatchAccumulator( batches, mesh.batchType );
		if( !accumulator )
		{
			for( auto& group : meshInfo.meshGroups )
			{
				group.pickingObjectId = 0xffffffff;
			}
			continue;
		}
		for( auto& group : meshInfo.meshGroups )
		{
			for( auto& lodIndices : meshInfo.lodIndices )
			{
				lodIndices.clear();
			}

			BinVisibleInstances( mesh, meshInfo, group );

			bool hasVisibleInstances = false;
			for( auto& lodIndices : meshInfo.lodIndices )
			{
				if( !lodIndices.empty() )
				{
					hasVisibleInstances = true;
					break;
				}
			}
			if( !hasVisibleInstances )
			{
				group.pickingObjectId = 0xffffffff;
				continue;
			}

			group.pickingObjectId = objectIdOffset;

			for( uint32_t lod = 0; lod < static_cast<uint32_t>( meshInfo.lodIndices.size() ); ++lod )
			{
				if( meshInfo.lodIndices[lod].empty() )
				{
					continue;
				}
				InstanceBuffer::Allocation allocation = AllocateInstanceData( uint32_t( meshInfo.lodIndices[lod].size() ), mesh.isDynamic );
				UploadLodData( mesh, meshInfo, lod, allocation );

				auto lodData = mesh.geometry->GetMeshLod( mesh.meshIndex, int( lod ) );
				auto primCount = GetPrimitiveCount( *lodData, mesh.areaIndex, mesh.areaCount );
				if( !primCount )
				{
					continue;
				}

				uint32_t stride = uint32_t( mesh.isDynamic ? sizeof( DynamicPerInstanceBufferElement ) : sizeof( StaticPerInstanceBufferElement ) );

				Tr2RenderBatch batch;
				batch.SetMaterial( meshInfo.material );
				batch.SetGeometry( mesh.combinedVertexDeclaration, lodData->m_vertexAllocation, lodData->m_indexAllocation );
				batch.SetStreamSource( 1, *allocation.buffer, stride );
				batch.SetDrawIndexedInstanced(
					primCount * 3,
					static_cast<uint32_t>( meshInfo.lodIndices[lod].size() ),
					lodData->m_indexAllocation.GetStartIndex() + lodData->m_areas[mesh.areaIndex].m_firstIndex,
					lodData->m_vertexAllocation.GetOffset() / lodData->m_vertexAllocation.GetStride(),
					allocation.offset / stride );

				auto perObjectData = accumulator->Allocate<PickingPerObjectData>();
				perObjectData->SetUserData( objectIdOffset + lod );
				batch.SetPerObjectData( perObjectData );

				accumulator->Commit( batch );

				traceback.push_back( { group.owner, group.ownerIndex } );
			}
			objectIdOffset += static_cast<uint32_t>( meshInfo.lodIndices.size() );
		}
	}
	m_staticInstanceBuffer.DoneCopying();
	m_dynamicInstanceBuffer.DoneCopying();
}

size_t EveInstancedMeshManager::GetBatches( const std::initializer_list<std::pair<TriBatchType, ITriRenderBatchAccumulator&>>& batches )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	BinVisibleInstances( batches );

	for( auto& [mesh, meshInfo] : m_meshInstances )
	{
		if( meshInfo.totalVisibleInstances != 0 )
		{
			meshInfo.currentAllocation = AllocateInstanceData( meshInfo.totalVisibleInstances, mesh.isDynamic );
		}
	}

	for( auto& [mesh, meshInfo] : m_meshInstances )
	{
		if( meshInfo.totalVisibleInstances == 0 )
		{
			continue;
		}

		for( uint32_t lod = 0; lod < static_cast<uint32_t>( meshInfo.lodIndices.size() ); ++lod )
		{
			if( !meshInfo.lodIndices[lod].empty() )
			{
				UploadLodData( mesh, meshInfo, lod, meshInfo.currentAllocation );
			}
		}
	}

	size_t totalBatches = 0;
	size_t totalInstances = 0;
	for( auto& [mesh, meshInfo] : m_meshInstances )
	{
		auto accumulator = FindBatchAccumulator( batches, mesh.batchType );
		if( !accumulator )
		{
			continue;
		}
		if( meshInfo.totalVisibleInstances == 0 )
		{
			continue;
		}

		auto& allocation = meshInfo.currentAllocation;
		uint32_t stride = uint32_t( mesh.isDynamic ? sizeof( DynamicPerInstanceBufferElement ) : sizeof( StaticPerInstanceBufferElement ) );
		auto instanceOffset = allocation.offset / stride;

		for( uint32_t lod = 0; lod < static_cast<uint32_t>( meshInfo.lodIndices.size() ); ++lod )
		{
			if( meshInfo.lodIndices[lod].empty() )
			{
				continue;
			}

			auto lodData = mesh.geometry->GetMeshLod( mesh.meshIndex, int( lod ) );

			auto primCount = GetPrimitiveCount( *lodData, mesh.areaIndex, mesh.areaCount );
			if( !primCount )
			{
				continue;
			}

			Tr2RenderBatch batch;
			batch.SetMaterial( meshInfo.material );
			batch.SetGeometry( mesh.combinedVertexDeclaration, lodData->m_vertexAllocation, lodData->m_indexAllocation );
			batch.SetStreamSource( 1, *allocation.buffer, stride );
			batch.SetDrawIndexedInstanced(
				primCount * 3,
				static_cast<uint32_t>( meshInfo.lodIndices[lod].size() ),
				lodData->m_indexAllocation.GetStartIndex() + lodData->m_areas[mesh.areaIndex].m_firstIndex,
				lodData->m_vertexAllocation.GetOffset() / lodData->m_vertexAllocation.GetStride(),
				instanceOffset );

			accumulator->Commit( batch );
			instanceOffset += static_cast<uint32_t>( meshInfo.lodIndices[lod].size() );

			totalInstances += meshInfo.lodIndices[lod].size();
			++totalBatches;
		}
	}
	CCP_STATS_ADD( eimmBatchesCount, totalBatches );
	CCP_STATS_ADD( eimmInstancesRendered, totalInstances );
	m_staticInstanceBuffer.DoneCopying();
	m_dynamicInstanceBuffer.DoneCopying();
	return totalBatches;
}

void EveInstancedMeshManager::UploadLodData( const MeshKey& mesh, MeshData& meshInfo, uint32_t lod, InstanceBuffer::Allocation& allocation )
{
	if( mesh.isDynamic )
	{
		for( auto [instance, perObjectDataIndex] : meshInfo.lodIndices[lod] )
		{
			memcpy( allocation.data, instance, sizeof( DynamicPerInstanceBufferElement ) );
			reinterpret_cast<DynamicPerInstanceBufferElement*>( allocation.data )->perObjectDataIndex = perObjectDataIndex;
			allocation.data += sizeof( DynamicPerInstanceBufferElement );
		}
	}
	else
	{
		for( auto [instance, perObjectDataIndex] : meshInfo.lodIndices[lod] )
		{
			memcpy( allocation.data, instance, sizeof( StaticPerInstanceBufferElement ) );
			reinterpret_cast<StaticPerInstanceBufferElement*>( allocation.data )->perObjectDataIndex = perObjectDataIndex;
			allocation.data += sizeof( StaticPerInstanceBufferElement );
		}
	}
}

EveInstancedMeshManager::InstanceBuffer::Allocation EveInstancedMeshManager::AllocateInstanceData( uint32_t count, bool isDynamic )
{
	if( isDynamic )
	{
		return m_dynamicInstanceBuffer.Allocate( count * sizeof( DynamicPerInstanceBufferElement ) );
	}
	return m_staticInstanceBuffer.Allocate( count * sizeof( StaticPerInstanceBufferElement ) );
}

uint32_t EveInstancedMeshManager::GetMeshLod( const MeshData& meshInfo, float screenSize )
{
	auto lodCount = static_cast<uint32_t>( meshInfo.lodIndices.size() );
	for( uint32_t lod = 0; lod + 1 < lodCount; ++lod )
	{
		if( screenSize > meshInfo.screenSizeThresholds[lod + 1] )
		{
			return lod;
		}
	}
	return lodCount - 1;
}

void EveInstancedMeshManager::ReportUsedScreenSizes() const
{
	for( auto& [mesh, meshInfo] : m_meshInstances )
	{
		if( meshInfo.maxScreenSize > 0.0f )
		{
			if( auto meshData = mesh.geometry->GetMeshLod( mesh.meshIndex, 0 ) )
			{
				meshInfo.material->UsedWithScreenSize( meshInfo.maxScreenSize, meshInfo.radius, meshData->m_uvDensities );
			}
		}
	}
}

void EveInstancedMeshManager::InstanceFlags::AddBatchType( TriBatchType type )
{
	m_flags |= 1 << static_cast<uint32_t>( type );
}

void EveInstancedMeshManager::InstanceFlags::SetCastsShadow( bool castsShadow )
{
	if( castsShadow )
	{
		m_flags |= CASTS_SHADOW;
	}
	else
	{
		m_flags &= ~CASTS_SHADOW;
	}
}

bool EveInstancedMeshManager::InstanceFlags::GetCastsShadow() const
{
	return ( m_flags & CASTS_SHADOW ) != 0;
}

void EveInstancedMeshManager::InstanceFlags::SetRenderInReflections( bool renderInReflections )
{
	if( renderInReflections )
	{
		m_flags |= RENDER_IN_REFLECTION;
	}
	else
	{
		m_flags &= ~RENDER_IN_REFLECTION;
	}
}

bool EveInstancedMeshManager::InstanceFlags::MatchesFilter( const InstanceFlags& filter ) const
{
	auto renderPassFlags = CASTS_SHADOW | RENDER_IN_REFLECTION;
	auto batchFilter = filter.m_flags & ~renderPassFlags;
	auto renderFlagsFilter = filter.m_flags & renderPassFlags;
	return ( m_flags & batchFilter ) != 0 && ( m_flags & renderFlagsFilter ) == renderFlagsFilter;
}

bool EveInstancedMeshManager::InstanceFlags::operator==( const InstanceFlags& other ) const
{
	return m_flags == other.m_flags;
}

bool EveInstancedMeshManager::InstanceFlags::operator!=( const InstanceFlags& other ) const
{
	return m_flags != other.m_flags;
}
