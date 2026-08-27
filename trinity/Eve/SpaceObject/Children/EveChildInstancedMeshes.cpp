// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveChildInstancedMeshes.h"
#include "./Tr2RingBuffer.h"
#include "../EveSpaceObject2.h"


EveChildInstancedMeshes::EveChildInstancedMeshes( IRoot* lockobj )
{
}

EveChildInstancedMeshes::~EveChildInstancedMeshes()
{
	for( Mesh& mesh : m_meshes )
	{
		if( mesh.geometry )
		{
			auto cur = begin( m_meshes ) + ( &mesh - m_meshes.data() );
			auto seenMesh = find_if( begin( m_meshes ), cur, [&]( const Mesh& m ) { return m.geometry == mesh.geometry; } ) != cur;
			if( !seenMesh )
			{
				mesh.geometry->RemoveNotifyTarget( this );
			}
		}
		for( auto& lod : mesh.rtMeshes )
		{
			delete lod.rtMesh;
			for( auto& area : lod.rtMeshAreas )
			{
				delete area.rtMeshArea;
			}
		}
	}
}

void EveChildInstancedMeshes::RegisterComponents()
{
	if( auto registry = GetComponentRegistry() )
	{
		registry->RegisterComponent<IEveInstanceMeshProvider>( this );
		registry->RegisterComponent<IEveShadowCaster>( this );
	}
}

void EveChildInstancedMeshes::UnRegisterComponents()
{
	UnregisterFromMeshManager();
}

void EveChildInstancedMeshes::UnregisterFromMeshManager()
{
	for( Mesh& mesh : m_meshes )
	{
		for( auto& area : mesh.areas )
		{
			if( area.meshGroupHandle )
			{
				area.meshGroupHandle.owner->RemoveMeshGroup( area.meshGroupHandle );
			}
		}
		if( mesh.sphereHandle )
		{
			mesh.sphereHandle.owner->RemoveBoundingSphereGroup( mesh.sphereHandle );
		}
	}
	if( m_perObjectDataHandle )
	{
		m_perObjectDataHandle.owner->RemovePerObjectData( m_perObjectDataHandle );
	}
	if( m_perObjectDataNoClipHandle )
	{
		m_perObjectDataNoClipHandle.owner->RemovePerObjectData( m_perObjectDataNoClipHandle );
	}
	m_allRegistered = false;
}

bool EveChildInstancedMeshes::IsCastingShadow( const TriFrustum& cameraFrustum, const IEveShadowFrustum& shadowFrustum, Tr2RenderReason renderReason, float& sizeInShadow ) const
{
	return false;
}

void EveChildInstancedMeshes::GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData, float shadowPixelSize )
{
}

Tr2PerObjectData* EveChildInstancedMeshes::GetShadowPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	return nullptr;
}

void EveChildInstancedMeshes::PushRtGeometry( Tr2RaytracingManager& rtManager ) const
{
	if( !m_hasUpdated )
	{
		return;
	}
	USE_MAIN_THREAD_RENDER_CONTEXT();
	EveSpaceObjectPSData psData = {};

	psData.worldTransform = m_perObjectData.worldTransform;
	psData.worldTransformLast = m_perObjectData.worldTransformLast;
	psData.invWorldTransform = m_perObjectData.invWorldTransform;
	psData.shipData = m_perObjectData.shipData;
	psData.clipSphereCenter = m_perObjectData.clipSphereCenter;
	psData.clipRadiusSq = m_perObjectData.clipRadiusSq;
	psData.clipRadius2Sq = m_perObjectData.clipRadius2Sq;
	psData.impactDataOffset = m_perObjectData.impactDataOffset;
	psData.clipSphereFactor2 = m_perObjectData.clipSphereFactor2;
	psData.clipSphereFactor = m_perObjectData.clipSphereFactor;
	for( int i = 0; i < EVE_SPACEOBJECT_CUSTOWMASK_MAX; ++i )
	{
		psData.customMaskMaterialIDs[i] = m_perObjectData.customMaskMaterialIDs[i];
		psData.customMaskTargets[i] = m_perObjectData.customMaskTargets[i];
	}
	psData.customMaskClamps = m_perObjectData.customMaskClamps;
	psData.customData = m_perObjectData.customData;

	UpdateRtPerObjectData( psData, nullptr, renderContext, m_rtPerObjectData );

	for( auto& mesh : m_meshes )
	{
		if( mesh.rtMeshes.empty() )
		{
			continue;
		}

		for( auto& lod : mesh.rtMeshes )
		{
			lod.instanceWorldTransforms.clear();
			lod.instanceWorldTransforms.reserve( mesh.instances.size() );
		}

		for( const auto& instanceTransform : mesh.instances )
		{
			float screenSize = m_lastCameraFrustum.GetPixelSizeAccross( mesh.instanceSpheres[&instanceTransform - mesh.instances.data()] );
			uint32_t lodIndex = 0;
			int32_t lastLod = static_cast<int32_t>( mesh.rtMeshes.size() ) - 1;
			for( int32_t i = lastLod; i >= 0; i-- )
			{
				if( mesh.rtMeshes[i].maxScreenSize >= screenSize )
				{
					lodIndex = static_cast<uint32_t>( i );
					break;
				}
			}

			XMMATRIX m = *reinterpret_cast<const Matrix*>( instanceTransform.worldTransform );
			m.r[3] = XMVectorSet( 0, 0, 0, 1 );
			m = XMMatrixMultiply( XMMatrixTranspose( m ), m_worldTransform );
			mesh.rtMeshes[lodIndex].instanceWorldTransforms.push_back( Float4x3( Matrix( m ) ) );
		}

		for( auto& lod : mesh.rtMeshes )
		{
			if( lod.instanceWorldTransforms.empty() )
			{
				continue;
			}
			for( auto& area : lod.rtMeshAreas )
			{
				Tr2Effect* effect = mesh.areas[area.areaIndex].effect;
				const Tr2ConstantBufferAL* vertexBufferData = nullptr;
				if( auto shader = effect->GetShaderStateInterface() )
				{
					if( shader->HasVertexBufferAccessInRtShadow() )
					{
						vertexBufferData = area.rtMeshArea->GetGeometryConstants( *lod.rtMesh, renderContext );
					}
				}
				rtManager.GetGeometry().AddGeometry( *lod.rtMesh, *area.rtMeshArea, effect, &m_rtPerObjectData, vertexBufferData, lod.instanceWorldTransforms.data(), lod.instanceWorldTransforms.size() );
			}
		}
	}
}


void EveChildInstancedMeshes::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod )
{
	m_lastCameraFrustum = updateContext.GetFrustum();
	m_lastInvLodFactor = updateContext.GetInvLodFactor();
}

void EveChildInstancedMeshes::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	// only overlay draws render here; the base hull goes through EveInstancedMeshManager
	if( m_hasUpdated && ( ( m_parentOverlayEffects != nullptr && AnyMeshInheritsOverlayEffects() ) || HasAnyOwnOverlayEffects() ) )
	{
		renderables.push_back( this );
	}
}

bool EveChildInstancedMeshes::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	return false;
}

void EveChildInstancedMeshes::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	m_worldTransform = params.localToWorldTransform;

	// overlay effect curves must update on the game thread; they may reference attributes that are not thread safe
	Be::Time time = updateContext.GetTime();
	for( Mesh& mesh : m_meshes )
	{
		for( EveMeshOverlayEffectPtr& overlay : mesh.ownOverlayEffects )
		{
			overlay->Update( time, time );
		}
	}
}

void EveChildInstancedMeshes::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	EveSpaceObjectPSData psData = {};
	EveSpaceObjectVSData vsData = {};
	if( params.spaceObjectParent )
	{
		params.spaceObjectParent->GetPerObjectStructs( vsData, psData );
	}

	m_perObjectData.worldTransformLast = m_perObjectData.worldTransform;

	m_perObjectData.worldTransform = Transpose( m_worldTransform );
	m_perObjectData.invWorldTransform = Inverse( m_perObjectData.worldTransform );

	m_perObjectData.shipData = psData.shipData;
	m_perObjectData.clipSphereCenter = psData.clipSphereCenter;
	m_perObjectData.clipRadiusSq = psData.clipRadiusSq;
	m_perObjectData.clipRadius2Sq = psData.clipRadius2Sq;
	m_perObjectData.impactDataOffset = psData.impactDataOffset;
	m_perObjectData.clipSphereFactor2 = psData.clipSphereFactor2;
	m_perObjectData.clipSphereFactor = psData.clipSphereFactor;
	m_perObjectData.ellpsoidRadii = vsData.ellpsoidRadii;
	m_perObjectData.ellpsoidCenter = vsData.ellpsoidCenter;
	for( int i = 0; i < EVE_SPACEOBJECT_CUSTOWMASK_MAX; ++i )
	{
		m_perObjectData.customMaskMatrix[i] = vsData.customMaskMatrix[i];
		m_perObjectData.customMaskData[i] = vsData.customMaskData[i];
		m_perObjectData.customMaskMaterialIDs[i] = psData.customMaskMaterialIDs[i];
		m_perObjectData.customMaskTargets[i] = psData.customMaskTargets[i];
	}
	m_perObjectData.customMaskClamps = psData.customMaskClamps;
	for( int i = 0; i < 4; ++i )
	{
		m_perObjectData.boneOffsets[i] = vsData.boneOffsets[i];
	}
	m_perObjectData.customData = vsData.customData;
	std::copy( std::begin( psData.shLightingCoefficients ), std::end( psData.shLightingCoefficients ), std::begin( m_perObjectData.shLighting ) );

	m_perObjectDataNoClip = m_perObjectData;
	m_perObjectDataNoClip.clipRadiusSq = 0.f;
	m_perObjectDataNoClip.clipRadius2Sq = 0.f;
	m_perObjectDataNoClip.clipSphereFactor = 0.f;
	m_perObjectDataNoClip.clipSphereFactor2 = 0.f;

	float worldScale = std::sqrtf( std::max( { LengthSq( Vector3( m_worldTransform._11, m_worldTransform._12, m_worldTransform._13 ) ),
											   LengthSq( Vector3( m_worldTransform._21, m_worldTransform._22, m_worldTransform._23 ) ),
											   LengthSq( Vector3( m_worldTransform._31, m_worldTransform._32, m_worldTransform._33 ) ) } ) );

	for( Mesh& mesh : m_meshes )
	{
		if( !mesh.geometry || !mesh.geometry->IsGood() )
		{
			continue;
		}

		mesh.flags.SetRenderInReflections( EntityComponents::ShouldReflect( mesh.reflectionMode ) );

		auto meshData = mesh.geometry->GetMeshData( mesh.meshIndex );
		auto sphere = CcpMath::Sphere( CcpMath::AxisAlignedBox( meshData->m_minBounds, meshData->m_maxBounds ) );
		float radius = sphere.radius + Length( sphere.center );

		CcpMath::AxisAlignedBox worldBounds;

		for( const auto& instance : mesh.instances )
		{
			Vector3 position = Vector3( instance.worldTransform[0].w, instance.worldTransform[1].w, instance.worldTransform[2].w );
			float scale = std::sqrtf( std::max( { LengthSq( instance.worldTransform[0].GetXYZ() ),
												  LengthSq( instance.worldTransform[1].GetXYZ() ),
												  LengthSq( instance.worldTransform[2].GetXYZ() ) } ) );
			position = TransformCoord( position, m_worldTransform );
			scale *= worldScale;
			mesh.instanceSpheres[&instance - mesh.instances.data()] = CcpMath::Sphere( position, radius * scale );
			worldBounds.Include( CcpMath::Sphere( position, radius * scale ) );
		}
		mesh.worldBoundingSphere = CcpMath::Sphere( worldBounds );

		if( !mesh.instanceSpheres.empty() )
		{
			if( mesh.sphereHandle )
			{
				mesh.sphereHandle.owner->SetSphereGroupBounds( mesh.sphereHandle, mesh.worldBoundingSphere, mesh.flags );
			}
		}
	}

	m_parentOverlayEffects = nullptr;
	if( params.spaceObjectParent )
	{
		if( EveSpaceObject2Ptr spaceObject2Parent = BlueCastPtr( params.spaceObjectParent ) )
		{
			const PEveMeshOverlayEffectVector& parentOverlays = spaceObject2Parent->GetOverlayEffects();
			if( !parentOverlays.empty() )
			{
				m_parentOverlayEffects = &parentOverlays;
			}
		}
	}
	if( ( m_parentOverlayEffects != nullptr && AnyMeshInheritsOverlayEffects() ) || HasAnyOwnOverlayEffects() )
	{
		UpdateOverlayInstanceData( vsData, psData );
	}

	m_hasUpdated = true;

	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( !renderContext.GetCaps().SupportsRaytracing() )
	{
		return;
	}

	for( Mesh& mesh : m_meshes )
	{
		if( !mesh.rtMeshes.empty() )
		{
			continue;
		}
		if( !mesh.flags.GetCastsShadow() )
		{
			continue;
		}
		if( !mesh.geometry || !mesh.geometry->IsGood() )
		{
			continue;
		}

		auto meshData = mesh.geometry->GetMeshData( mesh.meshIndex );
		mesh.rtMeshes.resize( meshData->m_lods.size() );

		for( auto& lod : mesh.rtMeshes )
		{
			if( !lod.rtMesh )
			{
				lod.rtMesh = new Tr2RaytracingMesh();
			}
			auto index = &lod - mesh.rtMeshes.data();
			lod.maxScreenSize = meshData->m_lods[index]->m_maxScreenSize;
			lod.rtMesh->UpdateRtMesh( mesh.geometry, mesh.meshIndex, lod.maxScreenSize );
			for( auto& area : mesh.areas )
			{
				if( area.batchType != TRIBATCHTYPE_OPAQUE )
				{
					continue;
				}
				auto& lodArea = lod.rtMeshAreas.emplace_back();
				lodArea.areaIndex = static_cast<uint32_t>( &area - mesh.areas.data() );
				lodArea.rtMeshArea = new Tr2RaytracingMeshArea( area.areaIndex );
			}
		}
	}
}

void EveChildInstancedMeshes::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

void EveChildInstancedMeshes::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
}

void EveChildInstancedMeshes::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	for( auto& mesh : m_meshes )
	{
		for( auto& area : mesh.areas )
		{
			area.effect->SetOption( name, value );
			area.effectHash = area.effect->GetHashValue();

			if( area.meshGroupHandle )
			{
				area.meshGroupHandle.owner->RemoveMeshGroup( area.meshGroupHandle );
				m_allRegistered = false;
			}
		}
	}
}


void EveChildInstancedMeshes::AddMesh(
	const char* geometryPath,
	bool castsShadow,
	EntityComponents::ReflectionMode reflectionMode,
	uint32_t meshIndex,
	const MeshArea* areas,
	size_t areaCount,
	const Matrix* instanceTransforms,
	size_t count,
	const BlueSharedString& sofHullName,
	const BlueSharedString& sofLocatorSetName,
	EveSpaceObjectChild::PartTag partTag )
{
	if( areaCount == 0 || count == 0 )
	{
		return;
	}

	for( auto& mesh : m_meshes )
	{
		if( mesh.geometryPath != geometryPath || mesh.meshIndex != meshIndex )
		{
			continue;
		}
		if( mesh.flags.GetCastsShadow() != castsShadow || mesh.reflectionMode != reflectionMode )
		{
			continue;
		}
		if( mesh.areas.size() != areaCount )
		{
			continue;
		}
		bool areasEqual = true;
		for( size_t i = 0; i < areaCount; ++i )
		{
			if( strcmp( mesh.areas[i].effect->GetEffectPathName(), areas[i].effect->GetEffectPathName() ) != 0 || mesh.areas[i].batchType != areas[i].batchType || mesh.areas[i].areaIndex != areas[i].areaIndex || mesh.areas[i].areaCount != areas[i].areaCount )
			{
				areasEqual = false;
				break;
			}
			if( mesh.areas[i].effectHash != areas[i].effect->GetHashValue() )
			{
				areasEqual = false;
				break;
			}
		}
		if( !areasEqual )
		{
			continue;
		}
		if( !( mesh.sofHullName == sofHullName && mesh.sofLocatorSetName == sofLocatorSetName ) )
		{
			continue;
		}
		const size_t existingCount = mesh.instances.size();
		mesh.instances.reserve( existingCount + count );
		mesh.partTags.reserve( mesh.partTags.size() + count );
		for( size_t i = 0; i < count; ++i )
		{
			EveInstancedMeshManager::StaticPerInstanceData instanceData;
			instanceData.SetTransform( instanceTransforms[i] );
			instanceData.sphereIndex = static_cast<uint32_t>( existingCount + i );
			mesh.instances.push_back( instanceData );
			mesh.partTags.push_back( partTag );
		}
		mesh.instanceSpheres.resize( mesh.instances.size() );
		if( mesh.sphereHandle )
		{
			mesh.sphereHandle.owner->RemoveBoundingSphereGroup( mesh.sphereHandle );
		}
		for( auto& area : mesh.areas )
		{
			if( area.meshGroupHandle )
			{
				area.meshGroupHandle.owner->RemoveMeshGroup( area.meshGroupHandle );
			}
		}
		m_allRegistered = false;
		return;
	}

	Mesh& mesh = m_meshes.emplace_back();
	mesh.geometryPath = geometryPath;
	mesh.reflectionMode = reflectionMode;
	mesh.meshIndex = meshIndex;
	for( size_t i = 0; i < areaCount; ++i )
	{
		auto& a = mesh.areas.emplace_back();
		a.effect = areas[i].effect;
		a.batchType = areas[i].batchType;
		a.areaIndex = areas[i].areaIndex;
		a.areaCount = areas[i].areaCount;
		a.alphaCutout = areas[i].alphaCutout;
		a.reversed = areas[i].reversed;
		a.effectHash = a.effect ? a.effect->GetHashValue() : 0;
	}
	mesh.instances.reserve( count );
	mesh.partTags.reserve( count );
	for( size_t i = 0; i < count; ++i )
	{
		EveInstancedMeshManager::StaticPerInstanceData instanceData;
		instanceData.SetTransform( instanceTransforms[i] );
		instanceData.sphereIndex = static_cast<uint32_t>( i );
		mesh.instances.push_back( instanceData );
		mesh.partTags.push_back( partTag );
	}
	mesh.instanceSpheres.resize( count );
	BeResMan->GetResource( mesh.geometryPath, "", mesh.geometry );
	if( mesh.geometry )
	{
		auto seenMesh = find_if( begin( m_meshes ), end( m_meshes ) - 1, [&]( const Mesh& m ) { return m.geometry == mesh.geometry; } );
		if( seenMesh == end( m_meshes ) - 1 )
		{
			mesh.geometry->AddNotifyTarget( this );
		}
		else
		{
			mesh.combinedVertexDeclaration = seenMesh->combinedVertexDeclaration;
		}
	}

	if( castsShadow )
	{
		mesh.flags.SetCastsShadow( true );
	}
	for( auto& area : mesh.areas )
	{
		mesh.flags.AddBatchType( area.batchType );
	}
	mesh.sofHullName = sofHullName;
	mesh.sofLocatorSetName = sofLocatorSetName;
	m_allRegistered = false;
}

void EveChildInstancedMeshes::RemoveInstancesByPartTag( EveSpaceObjectChild::PartTag partTag )
{
	for( size_t i = 0; i < m_meshes.size(); ++i )
	{
		auto& mesh = m_meshes[i];

		auto newEnd = std::remove_if( begin( mesh.instances ), end( mesh.instances ), [&]( const EveInstancedMeshManager::StaticPerInstanceData& instance ) {
			return mesh.partTags[&instance - mesh.instances.data()] == partTag;
		} );
		bool removed = newEnd != end( mesh.instances );
		if( !removed )
		{
			continue;
		}
		if( newEnd == begin( mesh.instances ) )
		{
			if( mesh.sphereHandle )
			{
				mesh.sphereHandle.owner->RemoveBoundingSphereGroup( mesh.sphereHandle );
			}
			for( auto& area : mesh.areas )
			{
				if( area.meshGroupHandle )
				{
					area.meshGroupHandle.owner->RemoveMeshGroup( area.meshGroupHandle );
				}
			}
			TriGeometryResPtr geometry = mesh.geometry;
			std::swap( mesh, m_meshes.back() );
			auto seenMesh = find_if( begin( m_meshes ), end( m_meshes ) - 1, [&]( const Mesh& m ) { return m.geometry == geometry; } );
			if( geometry && seenMesh == end( m_meshes ) - 1 )
			{
				geometry->RemoveNotifyTarget( this );
			}
			m_meshes.pop_back();
			--i;
			continue;
		}
		mesh.instances.erase( newEnd, end( mesh.instances ) );
		auto newTagEnd = std::remove_if( begin( mesh.partTags ), end( mesh.partTags ), [&]( uint32_t tag ) {
			return tag == partTag;
		} );
		mesh.partTags.erase( newTagEnd, end( mesh.partTags ) );
		for( auto& instance : mesh.instances )
		{
			instance.sphereIndex = static_cast<uint32_t>( &instance - mesh.instances.data() );
		}
		if( removed )
		{
			m_allRegistered = false;
			mesh.instanceSpheres.resize( mesh.instances.size() );
			if( mesh.sphereHandle )
			{
				mesh.sphereHandle.owner->RemoveBoundingSphereGroup( mesh.sphereHandle );
			}
			for( auto& area : mesh.areas )
			{
				if( area.meshGroupHandle )
				{
					area.meshGroupHandle.owner->RemoveMeshGroup( area.meshGroupHandle );
				}
			}
		}
	}
}
void EveChildInstancedMeshes::SetInstanceTransformByPartTag( PartTag partTag, const Vector3& translation, const Quaternion& rotation, Vector3 scale )
{
	Matrix m = TransformationMatrix( scale, rotation, translation );
	for( auto& mesh : m_meshes )
	{
		for( size_t i = 0; i < mesh.instances.size(); ++i )
		{
			if( mesh.partTags[i] == partTag )
			{
				mesh.instances[i].SetTransform( m );
			}
		}
	}
}

void EveChildInstancedMeshes::ReleaseCachedData( BlueAsyncRes* p )
{
}

void EveChildInstancedMeshes::RebuildCachedData( BlueAsyncRes* p )
{
	for( auto& mesh : m_meshes )
	{
		if( mesh.geometry == p )
		{
			mesh.combinedVertexDeclaration = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;

			if( auto data = mesh.geometry->GetMeshData( mesh.meshIndex ) )
			{
				Tr2VertexDefinition elements;
				if( Tr2EffectStateManager::GetVertexDeclarationElements( data->m_vertexDeclarationHandle, elements ) )
				{
					elements.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 8, 1, 1 );
					elements.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 9, 1, 1 );
					elements.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 10, 1, 1 );
					elements.m_nextOffset[1] = 0;
					elements.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 11, 1, 1 );
					elements.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 12, 1, 1 );
					elements.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 13, 1, 1 );
					elements.Add( Tr2VertexDefinition::UINT32_1, Tr2VertexDefinition::TEXCOORD, 14, 1, 1 );
					mesh.combinedVertexDeclaration = Tr2EffectStateManager::GetVertexDeclarationHandle( elements );
				}
			}
			for( auto& lod : mesh.rtMeshes )
			{
				delete lod.rtMesh;
				for( auto& area : lod.rtMeshAreas )
				{
					delete area.rtMeshArea;
				}
			}
			mesh.rtMeshes.clear();
		}
	}
}

void EveChildInstancedMeshes::AddMeshesToManager( EveInstancedMeshManager& manager )
{
	if( !m_hasUpdated )
	{
		return;
	}
	if( m_perObjectDataHandle && m_perObjectDataHandle.owner != &manager )
	{
		UnregisterFromMeshManager();
	}
	if( m_allRegistered )
	{
		return;
	}

	if( !m_perObjectDataHandle )
	{
		manager.AddPerObjectData( m_perObjectDataHandle, &m_perObjectData );
	}

	m_allRegistered = true;
	for( Mesh& mesh : m_meshes )
	{
		if( !mesh.display )
		{
			continue;
		}
		if( !mesh.geometry || mesh.combinedVertexDeclaration == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
		{
			m_allRegistered = false;
			continue;
		}
		if( !mesh.geometry->IsGood() )
		{
			m_allRegistered = false;
			continue;
		}
		if( mesh.instances.empty() )
		{
			m_allRegistered = false;
			continue;
		}
		if( mesh.instances.size() != mesh.instanceSpheres.size() )
		{
			m_allRegistered = false;
			continue;
		}

		if( !mesh.inheritOverlayEffects && !m_perObjectDataNoClipHandle )
		{
			manager.AddPerObjectData( m_perObjectDataNoClipHandle, &m_perObjectDataNoClip );
		}

		if( !mesh.sphereHandle )
		{
			manager.AddBoundingSphereGroup( mesh.sphereHandle, mesh.worldBoundingSphere, mesh.flags, mesh.instanceSpheres.data(), static_cast<uint32_t>( mesh.instanceSpheres.size() ) );
		}

		for( auto& area : mesh.areas )
		{
			if( !area.meshGroupHandle )
			{
				if( !area.effect || !area.effect->GetShaderStateInterface() )
				{
					m_allRegistered = false;
					continue;
				}
				manager.AddMeshGroup(
					area.meshGroupHandle,
					mesh.geometry,
					mesh.combinedVertexDeclaration,
					area.batchType,
					mesh.meshIndex,
					area.areaIndex,
					area.areaCount,
					area.effect,
					area.effectHash,
					mesh.inheritOverlayEffects ? m_perObjectDataHandle : m_perObjectDataNoClipHandle,
					mesh.sphereHandle,
					mesh.instances.data(),
					uint32_t( mesh.instances.size() ),
					GetRawRoot(),
					uint32_t( &mesh - m_meshes.data() ) );
			}
		}
	}
}

void EveChildInstancedMeshes::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Shared Instanced Mesh Bounds" );
}

void EveChildInstancedMeshes::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( renderer.HasOption( GetRawRoot(), "Shared Instanced Mesh Bounds" ) )
	{
		for( const Mesh& mesh : m_meshes )
		{
			if( mesh.instanceSpheres.empty() )
			{
				continue;
			}
			renderer.DrawSphere( this, mesh.worldBoundingSphere.center, mesh.worldBoundingSphere.radius, 20, ITr2DebugRenderer2::Wireframe, Tr2DebugColor( 0xffaa8800, 0x22aa8800 ) );
			for( auto& sphere : mesh.instanceSpheres )
			{
				renderer.DrawSphere( this, sphere.center, sphere.radius, 10, ITr2DebugRenderer2::Wireframe, Tr2DebugColor( 0xff008800, 0x22008800 ) );
			}
		}
	}
}

BluePy EveChildInstancedMeshes::GetSofSourceLocator( uint32_t areaId ) const
{
	auto meshIndex = areaId >> 16;
	auto locatorIndex = areaId & 0xFFFF;

	if( meshIndex >= m_meshes.size() || m_meshes[meshIndex].sofHullName.empty() )
	{
		return BluePy( Py_None, true );
	}

	BluePy result = BluePy( PyTuple_New( 3 ) );
	PyTuple_SetItem( result, 0, ToPython( m_meshes[meshIndex].sofHullName.c_str() ) );
	PyTuple_SetItem( result, 1, ToPython( m_meshes[meshIndex].sofLocatorSetName.c_str() ) );
	PyTuple_SetItem( result, 2, ToPython( locatorIndex ) );
	return result;
}

uint32_t EveChildInstancedMeshes::GetMeshCount() const
{
	return static_cast<uint32_t>( m_meshes.size() );
}
BluePy EveChildInstancedMeshes::GetInstancesTransforms( uint32_t meshId ) const
{
	if( meshId >= m_meshes.size() )
	{
		PyErr_SetString( PyExc_IndexError, "Mesh index out of range" );
		return {};
	}

	auto& mesh = m_meshes[meshId];
	BluePy result( PyTuple_New( mesh.instances.size() ) );
	int i = 0;
	for( auto& instance : mesh.instances )
	{
		Vector3 scale, translation;
		Quaternion rotation;
		Decompose( scale, rotation, translation, instance.ToMatrix() );

		PyObject* transform = PyTuple_New( 3 );
		PyTuple_SetItem( transform, 0, ToPython( translation ) );
		PyTuple_SetItem( transform, 1, ToPython( rotation ) );
		PyTuple_SetItem( transform, 2, ToPython( scale ) );
		PyTuple_SetItem( result, i++, transform );
	}

	return result;
}

BluePy EveChildInstancedMeshes::GetMeshInfo( uint32_t meshId ) const
{
	if( meshId >= m_meshes.size() )
	{
		PyErr_SetString( PyExc_IndexError, "Mesh index out of range" );
		return {};
	}
	auto& mesh = m_meshes[meshId];

	BluePy result( PyTuple_New( 7 ) );
	PyTuple_SetItem( result, 0, ToPython( mesh.geometryPath.c_str() ) );
	PyTuple_SetItem( result, 1, ToPython( mesh.geometry.p ) );
	PyTuple_SetItem( result, 2, ToPython( mesh.meshIndex ) );
	PyTuple_SetItem( result, 3, ToPython( mesh.flags.GetCastsShadow() ) );
	PyTuple_SetItem( result, 4, ToPython( mesh.reflectionMode ) );
	PyTuple_SetItem( result, 5, ToPython( mesh.areas.size() ) );
	PyTuple_SetItem( result, 6, ToPython( mesh.instances.size() ) );

	return result;
}

BluePy EveChildInstancedMeshes::GetAreaInfo( uint32_t meshId, uint32_t areaId ) const
{
	if( meshId >= m_meshes.size() )
	{
		PyErr_SetString( PyExc_IndexError, "Mesh index out of range" );
		return {};
	}
	auto& mesh = m_meshes[meshId];
	if( areaId >= mesh.areas.size() )
	{
		PyErr_SetString( PyExc_IndexError, "Area index out of range" );
		return {};
	}
	auto& area = mesh.areas[areaId];

	BluePy result( PyTuple_New( 4 ) );
	PyTuple_SetItem( result, 0, ToPython( area.effect.p ) );
	PyTuple_SetItem( result, 1, ToPython( static_cast<uint32_t>( area.batchType ) ) );
	PyTuple_SetItem( result, 2, ToPython( area.areaIndex ) );
	PyTuple_SetItem( result, 3, ToPython( area.areaCount ) );
	return result;
}

BluePy EveChildInstancedMeshes::GetMeshDisplay( uint32_t meshId ) const
{
	if( meshId >= m_meshes.size() )
	{
		PyErr_SetString( PyExc_IndexError, "Mesh index out of range" );
		return {};
	}
	auto& mesh = m_meshes[meshId];
	if( mesh.display )
	{
		return BluePy( Py_True, true );
	}
	else
	{
		return BluePy( Py_False, true );
	}
}

BluePy EveChildInstancedMeshes::SetMeshDisplay( uint32_t meshId, bool display )
{
	if( meshId >= m_meshes.size() )
	{
		PyErr_SetString( PyExc_IndexError, "Mesh index out of range" );
		return {};
	}
	auto& mesh = m_meshes[meshId];
	if( mesh.display != display )
	{
		mesh.display = display;
		m_allRegistered = false;
		if( !mesh.display )
		{
			if( mesh.sphereHandle )
			{
				mesh.sphereHandle.owner->RemoveBoundingSphereGroup( mesh.sphereHandle );
			}
			for( auto& area : mesh.areas )
			{
				if( area.meshGroupHandle )
				{
					area.meshGroupHandle.owner->RemoveMeshGroup( area.meshGroupHandle );
				}
			}
		}
	}
	return BluePy( Py_None, true );
}

BluePy EveChildInstancedMeshes::GetMeshInheritOverlayEffects( uint32_t meshId ) const
{
	if( meshId >= m_meshes.size() )
	{
		PyErr_SetString( PyExc_IndexError, "Mesh index out of range" );
		return {};
	}
	if( m_meshes[meshId].inheritOverlayEffects )
	{
		return BluePy( Py_True, true );
	}
	else
	{
		return BluePy( Py_False, true );
	}
}

BluePy EveChildInstancedMeshes::SetMeshInheritOverlayEffects( uint32_t meshId, bool inherit )
{
	if( meshId >= m_meshes.size() )
	{
		PyErr_SetString( PyExc_IndexError, "Mesh index out of range" );
		return {};
	}
	auto& mesh = m_meshes[meshId];
	if( mesh.inheritOverlayEffects != inherit )
	{
		mesh.inheritOverlayEffects = inherit;
		m_allRegistered = false;
		for( auto& area : mesh.areas )
		{
			if( area.meshGroupHandle )
			{
				area.meshGroupHandle.owner->RemoveMeshGroup( area.meshGroupHandle );
			}
		}
	}
	return BluePy( Py_None, true );
}

void EveChildInstancedMeshes::ReleaseResources( TriStorage s )
{
	if( ( s & TRISTORAGE_MANAGEDMEMORY ) != 0 )
	{
		UnregisterFromMeshManager();
	}
}

bool EveChildInstancedMeshes::OnPrepareResources()
{
	return true;
}

BluePy EveChildInstancedMeshes::AddMeshOverlayEffect( uint32_t meshId, EveMeshOverlayEffect* overlayEffect )
{
	if( meshId >= m_meshes.size() )
	{
		PyErr_SetString( PyExc_IndexError, "Mesh index out of range" );
		return {};
	}
	if( overlayEffect == nullptr )
	{
		PyErr_SetString( PyExc_TypeError, "overlayEffect must not be None" );
		return {};
	}
	m_meshes[meshId].ownOverlayEffects.push_back( overlayEffect );
	return BluePy( Py_None, true );
}

BluePy EveChildInstancedMeshes::RemoveMeshOverlayEffect( uint32_t meshId, EveMeshOverlayEffect* overlayEffect )
{
	if( meshId >= m_meshes.size() )
	{
		PyErr_SetString( PyExc_IndexError, "Mesh index out of range" );
		return {};
	}
	auto& overlays = m_meshes[meshId].ownOverlayEffects;
	auto it = std::find( overlays.begin(), overlays.end(), EveMeshOverlayEffectPtr( overlayEffect ) );
	if( it != overlays.end() )
	{
		overlays.erase( it );
	}
	return BluePy( Py_None, true );
}

BluePy EveChildInstancedMeshes::ClearMeshOverlayEffects( uint32_t meshId )
{
	if( meshId >= m_meshes.size() )
	{
		PyErr_SetString( PyExc_IndexError, "Mesh index out of range" );
		return {};
	}
	m_meshes[meshId].ownOverlayEffects.clear();
	return BluePy( Py_None, true );
}

BluePy EveChildInstancedMeshes::GetMeshOverlayEffectCount( uint32_t meshId ) const
{
	if( meshId >= m_meshes.size() )
	{
		PyErr_SetString( PyExc_IndexError, "Mesh index out of range" );
		return {};
	}
	return BluePy( ToPython( uint32_t( m_meshes[meshId].ownOverlayEffects.size() ) ) );
}

bool EveChildInstancedMeshes::HasAnyOwnOverlayEffects() const
{
	for( const Mesh& mesh : m_meshes )
	{
		if( !mesh.ownOverlayEffects.empty() )
		{
			return true;
		}
	}
	return false;
}

bool EveChildInstancedMeshes::AnyMeshInheritsOverlayEffects() const
{
	for( const Mesh& mesh : m_meshes )
	{
		if( mesh.inheritOverlayEffects )
		{
			return true;
		}
	}
	return false;
}

bool EveChildInstancedMeshes::MeshHasActiveOverlayEffects( const Mesh& mesh ) const
{
	return !mesh.ownOverlayEffects.empty() || ( m_parentOverlayEffects != nullptr && mesh.inheritOverlayEffects );
}

uint32_t EveChildInstancedMeshes::OverlayInstancePod::GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const
{
	if( shaderType == Tr2RenderContextEnum::PIXEL_SHADER )
	{
		return sizeof( psData );
	}
	return sizeof( vsData );
}

void EveChildInstancedMeshes::OverlayInstancePod::UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* data )
{
	if( shaderType == Tr2RenderContextEnum::PIXEL_SHADER )
	{
		memcpy( data, &psData, sizeof( psData ) );
	}
	else
	{
		memcpy( data, &vsData, sizeof( vsData ) );
	}
}

void EveChildInstancedMeshes::UpdateOverlayInstanceData( const EveSpaceObjectVSData& parentVsData, const EveSpaceObjectPSData& parentPsData )
{
	// worldTransformLast still holds the previous frame's transposed world transform here
	Matrix prevWorldTransform = Transpose( m_perObjectData.worldTransformLast );

	for( Mesh& mesh : m_meshes )
	{
		if( mesh.instances.empty() || !mesh.display || !MeshHasActiveOverlayEffects( mesh ) )
		{
			continue;
		}
		if( !mesh.overlayPods )
		{
			// sized once; the pods own device resources so they must stay at stable addresses
			mesh.overlayPods = std::make_unique<std::vector<OverlayInstancePod>>( mesh.instances.size() );
		}

		for( size_t i = 0; i < mesh.instances.size(); ++i )
		{
			const auto& wt = mesh.instances[i].worldTransform;
			OverlayInstancePod& pod = ( *mesh.overlayPods )[i];

			Matrix local = mesh.instances[i].ToMatrix();

			Matrix worldTransform = Transpose( local * m_worldTransform );
			Matrix worldTransformLast = Transpose( local * prevWorldTransform );
			Matrix invWorldTransform = Inverse( worldTransform );
			Matrix invLocal = Inverse( local );

			pod.vsData = parentVsData;
			pod.vsData.worldTransform = worldTransform;
			pod.vsData.worldTransformLast = worldTransformLast;
			pod.vsData.invWorldTransform = invWorldTransform;
			// need to move the clipdata inversely of the transform of the instance
			pod.vsData.clipData = Vector4( TransformCoord( pod.vsData.clipData.GetXYZ(), invLocal ), pod.vsData.clipData.w );

			pod.psData = parentPsData;
			pod.psData.worldTransform = worldTransform;
			pod.psData.worldTransformLast = worldTransformLast;
			pod.psData.invWorldTransform = invWorldTransform;
			pod.psData.clipSphereCenter = TransformCoord( pod.psData.clipSphereCenter, invLocal );

			if( !mesh.inheritOverlayEffects )
			{
				// opted out of the parent's overlay: also neutralize the inherited clip sphere
				pod.vsData.clipData.w = 0.f;
				pod.psData.clipRadiusSq = 0.f;
				pod.psData.clipRadius2Sq = 0.f;
				pod.psData.clipSphereFactor = 0.f;
				pod.psData.clipSphereFactor2 = 0.f;
			}

			pod.vsBuffer.InvalidateBufferData();
			pod.psBuffer.InvalidateBufferData();
		}
	}
}

void EveChildInstancedMeshes::RebuildOverlayAreaBlocks( Mesh& mesh )
{
	for( int i = 0; i < EveMeshOverlayEffect::TYPE_COUNT; ++i )
	{
		mesh.overlayAreaBlocks[i].clear();
	}

	for( const MeshArea& area : mesh.areas )
	{
		if( area.batchType == TRIBATCHTYPE_OPAQUE || area.batchType == TRIBATCHTYPE_TRANSPARENT || area.batchType == TRIBATCHTYPE_DECAL )
		{
			mesh.overlayAreaBlocks[EveMeshOverlayEffect::TYPE_ALL].emplace_back( area.areaIndex, area.areaCount );
		}
		if( area.batchType == TRIBATCHTYPE_OPAQUE )
		{
			mesh.overlayAreaBlocks[EveMeshOverlayEffect::TYPE_OPAQUEONLY].emplace_back( area.areaIndex, area.areaCount );
		}
	}

	for( int i = 0; i < EveMeshOverlayEffect::TYPE_COUNT; ++i )
	{
		TriRenderBatchAreaBlock::Optimize( mesh.overlayAreaBlocks[i] );
	}
	mesh.overlayAreaBlocksBuilt = true;
}

bool EveChildInstancedMeshes::HasTransparentBatches()
{
	if( m_parentOverlayEffects != nullptr && AnyMeshInheritsOverlayEffects() )
	{
		for( const auto& overlayEffect : *m_parentOverlayEffects )
		{
			if( overlayEffect->HasTransparentArea() )
			{
				return true;
			}
		}
	}
	for( const Mesh& mesh : m_meshes )
	{
		for( const EveMeshOverlayEffectPtr& overlay : mesh.ownOverlayEffects )
		{
			if( overlay->HasTransparentArea() )
			{
				return true;
			}
		}
	}
	return false;
}

float EveChildInstancedMeshes::GetSortValue()
{
	Vector3 d = Tr2Renderer::GetViewPosition() - m_worldTransform.GetTranslation();
	return Length( d );
}

Tr2PerObjectData* EveChildInstancedMeshes::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	if( ( m_parentOverlayEffects == nullptr || !AnyMeshInheritsOverlayEffects() ) && !HasAnyOwnOverlayEffects() )
	{
		return nullptr;
	}

	Tr2PerObjectData* firstPod = nullptr;
	for( Mesh& mesh : m_meshes )
	{
		if( !mesh.overlayPods )
		{
			continue;
		}
		if( !mesh.display || !MeshHasActiveOverlayEffects( mesh ) )
		{
			for( OverlayInstancePod& pod : *mesh.overlayPods )
			{
				pod.framePod = nullptr;
			}
			continue;
		}
		for( OverlayInstancePod& pod : *mesh.overlayPods )
		{
			pod.framePod = nullptr;
			auto* perObjectData = accumulator->Allocate<Tr2PerObjectDataWithPersistentBuffers<OverlayInstancePod>>();
			if( perObjectData == nullptr )
			{
				return firstPod;
			}
			perObjectData->Initialize( &pod, &pod.vsBuffer, &pod.psBuffer );
			pod.framePod = perObjectData;
			if( firstPod == nullptr )
			{
				firstPod = perObjectData;
			}
		}
	}
	return firstPod;
}

void EveChildInstancedMeshes::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	if( !m_hasUpdated )
	{
		return;
	}

	for( Mesh& mesh : m_meshes )
	{
		const bool hasOwnOverlays = !mesh.ownOverlayEffects.empty();
		const bool hasInheritedOverlays = m_parentOverlayEffects != nullptr && mesh.inheritOverlayEffects;
		if( !hasInheritedOverlays && !hasOwnOverlays )
		{
			continue;
		}
		if( !mesh.display || !mesh.overlayPods )
		{
			continue;
		}
		if( reason == TR2RENDERREASON_REFLECTION && !EntityComponents::ShouldReflect( mesh.reflectionMode ) )
		{
			continue;
		}
		if( !mesh.geometry || !mesh.geometry->IsGood() )
		{
			continue;
		}
		if( !mesh.overlayAreaBlocksBuilt )
		{
			RebuildOverlayAreaBlocks( mesh );
		}

		for( size_t i = 0; i < mesh.overlayPods->size(); ++i )
		{
			OverlayInstancePod& pod = ( *mesh.overlayPods )[i];
			if( pod.framePod == nullptr )
			{
				continue;
			}

			// per-instance LOD selection matching the base hull in EveInstancedMeshManager
			const CcpMath::Sphere& sphere = mesh.instanceSpheres[i];
			if( !m_lastCameraFrustum.IsSphereVisible( sphere.center, sphere.radius ) )
			{
				continue;
			}
			float screenSize = m_lastCameraFrustum.GetPixelSizeAccrossEst( sphere.center, sphere.radius ) * m_lastInvLodFactor;
			auto lod = mesh.geometry->GetMeshLod( mesh.meshIndex, screenSize );
			if( !lod || !lod->m_allocationsValid )
			{
				continue;
			}

			// own effects are emitted before the inherited ones so the parent's overlays
			// (e.g. cloak) draw on top of this mesh's own overlays
			if( hasOwnOverlays )
			{
				EmitOverlayBatches( batches, pod.framePod, batchType, mesh.ownOverlayEffects, mesh.overlayAreaBlocks, *lod );
			}
			if( hasInheritedOverlays )
			{
				EmitOverlayBatches( batches, pod.framePod, batchType, *m_parentOverlayEffects, mesh.overlayAreaBlocks, *lod );
			}
		}
	}
}

void EveChildInstancedMeshes::CollectOwnedGeometry( TriBatchType type, const Matrix& parentTransform, std::vector<EveChildGeometry>& out, std::vector<EveChildGeometryArea>& areaPool ) const
{
	static_assert(
		sizeof( Float4x3 ) == sizeof( EveInstancedMeshManager::StaticPerInstanceData::worldTransform ),
		"Float4x3 must match StaticPerInstanceData::worldTransform" );

	for( const Mesh& mesh : m_meshes )
	{
		if( !mesh.geometry || mesh.instances.empty() )
		{
			continue;
		}

		uint32_t areaStart = uint32_t( areaPool.size() );
		for( const MeshArea& area : mesh.areas )
		{
			if( area.batchType != type )
			{
				continue;
			}
			EveChildGeometryArea childGeometryArea;
			childGeometryArea.index = area.areaIndex;
			childGeometryArea.count = area.areaCount;
			childGeometryArea.alphaCutout = area.alphaCutout;
			childGeometryArea.reversed = area.reversed;
			areaPool.push_back( childGeometryArea );
		}
		uint32_t areaCount = uint32_t( areaPool.size() ) - areaStart;

		if( areaCount == 0 )
		{
			continue;
		}

		for( const auto& instance : mesh.instances )
		{
			Matrix instanceTransform = *(Float4x3*)&instance.worldTransform;
			EveChildGeometry source;
			source.childToObject = instanceTransform * parentTransform;
			source.geometry = mesh.geometry;
			source.areaStart = areaStart;
			source.areaCount = areaCount;
			out.push_back( source );
		}
	}
}
