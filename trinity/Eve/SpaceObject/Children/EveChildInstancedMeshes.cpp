// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveChildInstancedMeshes.h"
#include "./Tr2RingBuffer.h"


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


const char* EveChildInstancedMeshes::GetName() const
{
	return m_name.c_str();
}

void EveChildInstancedMeshes::SetName( const char* name )
{
	m_name = BlueSharedString( name );
}

void EveChildInstancedMeshes::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod )
{
	m_lastCameraFrustum = updateContext.GetFrustum();
}

void EveChildInstancedMeshes::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
}

bool EveChildInstancedMeshes::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	return false;
}

void EveChildInstancedMeshes::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	m_worldTransform = params.localToWorldTransform;
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

void EveChildInstancedMeshes::ChangeLOD( Tr2Lod lod )
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
	const BlueSharedString& sofLocatorSetName )
{
	if( areaCount == 0 || count == 0 )
	{
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
		a.effectHash = a.effect ? a.effect->GetHashValue() : 0;
	}
	mesh.instances.reserve( count );
	for( size_t i = 0; i < count; ++i )
	{
		EveInstancedMeshManager::StaticPerInstanceData instanceData;
		auto& mat = instanceTransforms[i];
		instanceData.worldTransform[0] = Vector4( mat._11, mat._21, mat._31, mat._41 );
		instanceData.worldTransform[1] = Vector4( mat._12, mat._22, mat._32, mat._42 );
		instanceData.worldTransform[2] = Vector4( mat._13, mat._23, mat._33, mat._43 );
		instanceData.sphereIndex = static_cast<uint32_t>( i );
		mesh.instances.push_back( instanceData );
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
					m_perObjectDataHandle,
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
