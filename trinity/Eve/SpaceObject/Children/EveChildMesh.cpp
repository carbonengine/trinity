#include "StdAfx.h"
#include "EveChildMesh.h"
#include "Tr2MeshBase.h"
#include "TriFrustum.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Eve/EveTransform.h"
#include "Utilities/BoundingSphere.h"
#include "Tr2InstancedMesh.h"
#include "Tr2GrannyAnimation.h"
#include "TriFrustumOrtho.h"
#include "Utilities/BoundingBox.h"
#include "Resources/TriGeometryRes.h"
#include <IBlueObjectMetadata.h>

namespace
{
constexpr float s_instanceScreenSizeThreshold = 1.f;

}

EveChildMesh::EveChildMesh( IRoot* lockobj ):
	PARENTLOCK( m_transformModifiers ),
	PARENTLOCK( m_decals ),
	PARENTLOCK( m_attachments ),
	PARENTLOCK( m_lights ),
	m_display( true ),
	m_isVisible( false ),
	m_instancesVisible( false ),
	m_castShadow( false ),
	m_updateAnimation( true ),
	m_lowestLodVisible( TR2_LOD_LOW ),
	m_minScreenSize( 0.f ),
	m_currentScreenSize( -1.f ),
	m_currentInstanceScreenSize( -1.f ),
	m_sortValueOffset( 0 ),
	m_sortValueScale( 1 ),
	m_activationStrength( 1.0f ),
	m_origin( SPACE ),
	m_reflectionMode( EntityComponents::REFLECT_NEVER ),
	m_instanceCount( 0 ),
	EveChildTransform(),
	EveEntity( lockobj )
{
	// init per-object data with default values
	memset( &m_vsData, 0, sizeof( EveSpaceObjectVSData ) );
	memset( &m_psData, 0, sizeof( EveSpaceObjectPSData ) );
	m_vsData.shipData.y = 1.f;
	m_vsData.shipData.w = 1.f;
	m_psData.shipData.y = 1.f;
	m_psData.shipData.w = 1.f;
	m_psData.screenSize = Vector4( 0.5f, 0.5f, 0.5f, 1.f );

	m_decals.SetNotify( this );
	m_lights.SetNotify( this );
	m_attachments.SetNotify( this );
}

EveChildMesh::~EveChildMesh()
{
}

bool EveChildMesh::Initialize()
{
	if( m_staticTransform )
	{
		RebuildLocalTransform();
	}

	InitializeAnimation();

	for( uint32_t i = 0; i < m_decals.size(); i++ )
	{
		m_decals[i]->SetPriority( i );
	}

	return true;
}

void EveChildMesh::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list )
{
	if( list == &m_decals )
	{
		if( ( event & BELIST_EVENTMASK ) == BELIST_INSERTED && key == m_decals.size() )
		{
			// this is here in case someone calls the append function of bluelist from python. Remove this once the off by one error has been fixed!
			m_decals[m_decals.size() - 1]->SetPriority( (uint32_t)m_decals.size() - 1 );
		}
		else if( ( event & BELIST_EVENTMASK ) == BELIST_INSERTED )
		{
			for( size_t i = key; i < m_decals.size(); i++ )
			{
				m_decals[i]->SetPriority( (uint32_t)i );
			}
		}
		else if( ( event & BELIST_EVENTMASK ) == BELIST_REMOVED )
		{
			for( size_t i = key; i < m_decals.size(); i++ )
			{
				m_decals[i]->SetPriority( (uint32_t)i );
			}
		}
		else if( ( event & BELIST_EVENTMASK ) == BELIST_SWAPPED )
		{
			m_decals[key]->SetPriority( (uint32_t)key );
			m_decals[key2]->SetPriority( (uint32_t)key2 );
		}
		else if( ( event & BELIST_EVENTMASK ) == BELIST_MOVED )
		{
			size_t low = min( key, key2 );
			size_t high = max( key, key2 );
			for( size_t i = low; i <= high; i++ )
			{
				m_decals[i]->SetPriority( (uint32_t)i );
			}
		}
	}

	if ( list == &m_lights )
	{
		auto maskedEvent = event & BELIST_EVENTMASK;
		if( ( maskedEvent == BELIST_UNLOADSTART ) || ( ( maskedEvent == BELIST_REMOVED ) && m_lights.empty() ) )
		{
			auto registry = this->GetComponentRegistry();
			if( registry )
			{
				registry->UnRegisterComponent<ITr2LightOwner>( this );
			}
		}
		else if( ( maskedEvent == BELIST_INSERTED ) && m_lights.size() == 1 )
		{
			auto registry = this->GetComponentRegistry();
			if( registry )
			{
				registry->RegisterComponent<ITr2LightOwner>( this );
			}
		}
	}

	if( list == &m_attachments && ( event & BELIST_LOADING ) == 0 )
	{
		if( IsInRegistry() )
		{
			switch( event & BELIST_EVENTMASK )
			{
			case BELIST_INSERTED:
				if( EveEntityPtr entity = BlueCastPtr( value ) )
				{
					entity->Register( GetComponentRegistry() );
				}
				break;
			case BELIST_REMOVED:
				if( EveEntityPtr entity = BlueCastPtr( value ) )
				{
					entity->UnRegister( GetComponentRegistry() );
				}
				break;
			case BELIST_UNLOADSTART:
				for( ssize_t i = 0; i < list->GetSize(); ++i )
				{
					if( EveEntityPtr entity = BlueCastPtr( list->GetAt( i ) ) )
					{
						entity->UnRegister( GetComponentRegistry() );
					}
				}
				break;
			default:
				break;
			}
		}
	}
}

bool EveChildMesh::OnModified( Be::Var* val )
{
	if( IsMatch( val, m_reflectionMode ) || IsMatch( val, m_display) || IsMatch( val, m_mesh) || IsMatch( val, m_castShadow ) )
	{
		ReRegister();
	}
	if( IsMatch( val, m_mesh ) || IsMatch( val, m_animationUpdater ) )
	{
		m_instancedMesh = BlueCastPtr( m_mesh );
		InitializeAnimation();
	}
	return true;
}

const char* EveChildMesh::GetName() const
{
	return m_name.c_str();
}

void EveChildMesh::SetName( const char* name )
{
	m_name = BlueSharedString( name );
}

void EveChildMesh::InitializeAnimation()
{
	if( m_animationUpdater && m_animationUpdater->GetResPath().empty() )
	{
		if( m_mesh )
		{
			if( auto geometry = m_mesh->GetGeometryResource() )
			{
				m_animationUpdater->SetUseMeshBinding( true );
				m_animationUpdater->SetSharedGeometryRes( geometry );
				return;
			}
		}
		m_animationUpdater->SetSharedGeometryRes( nullptr );
	}
}


// --------------------------------------------------------------------------------
// Description:
//    Registers itself and its children with the scene registration container.
//    This is so we don't have to traverse the tree every frame
// --------------------------------------------------------------------------------
void EveChildMesh::RegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry && m_display  )
	{
		if( !m_lights.empty() )
		{
			registry->RegisterComponent<ITr2LightOwner>( this );
		}

		if( m_mesh != nullptr )
		{
			if( EntityComponents::ShouldReflect( m_reflectionMode ) )
			{
				registry->RegisterComponent<ITr2Renderable>( this );
			}
			if( m_castShadow )
			{
				registry->RegisterComponent<IEveShadowCaster>( this );
			}
		}

		for( auto it = std::begin( m_attachments ); it != std::end( m_attachments ); ++it )
		{
			if( EveEntityPtr entity = BlueCastPtr( *it ) )
			{
				entity->Register( registry );
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//    Unregisters itself and its children with the scene registration container.
// --------------------------------------------------------------------------------
void EveChildMesh::UnRegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry )
	{
		for( auto it = std::begin( m_attachments ); it != std::end( m_attachments ); ++it )
		{
			if( EveEntityPtr entity = BlueCastPtr( *it ) )
			{
				entity->UnRegister( registry );
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Check if the object is casting a shadow in the camera/shadow frustums
bool EveChildMesh::IsCastingShadow( const TriFrustum& cameraFrustum, const IEveShadowFrustum& shadowFrustum, Tr2RenderReason renderReason, float& sizeInShadow ) const
{
	if( !m_display || !m_castShadow )
	{
		return false;
	}

	if( renderReason == TR2RENDERREASON_REFLECTION && !EntityComponents::ShouldReflect( m_reflectionMode ) )
	{
		return false;
	}

	Vector4 bs = Vector4( m_worldBoundingSphere.center, m_worldBoundingSphere.radius );
	sizeInShadow = 0;

	if( bs.w <= 0.0f )
	{
		return false;
	}

	if( shadowFrustum.IsVisible( cameraFrustum, bs ) )
	{
		if( m_instancedMesh )
		{
			if( auto instanceBounds = m_instancedMesh->GetInstanceBoundsClosestToPoint( TransformCoord( shadowFrustum.GetEyePos(), Inverse( m_worldTransform ) ) ) )
			{
				instanceBounds.Transform( m_worldTransform );

				sizeInShadow = shadowFrustum.GetSizeInShadow( Vector4( instanceBounds.center, instanceBounds.radius ) );
			}
			else
			{
				sizeInShadow = shadowFrustum.GetSizeInShadow( bs );
			}
		}
		else
		{
			sizeInShadow = shadowFrustum.GetSizeInShadow( bs );	
		}
	}
	return sizeInShadow > 5.f;
}

void EveChildMesh::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod )
{
	m_isVisible = false;
	m_currentScreenSize = -1;
	m_instancesVisible = false;
	m_currentInstanceScreenSize = -1.0f;
	auto& frustum = updateContext.GetFrustum();
	auto invLodFactor = updateContext.GetInvLodFactor();

	if( m_mesh )
	{
		m_currentScreenSize = frustum.GetPixelSizeAccross( m_worldBoundingSphere );

		if( m_instancedMesh )
		{
			if( auto instanceBounds = m_instancedMesh->GetInstanceBoundsClosestToPoint( TransformCoord( frustum.m_viewPos, Inverse( m_worldTransform ) ) ) )
			{
				instanceBounds.Transform( m_worldTransform );
				m_currentInstanceScreenSize = frustum.GetPixelSizeAccross( instanceBounds );
				m_mesh->UseWithScreenSize( m_currentInstanceScreenSize, instanceBounds.radius );
			}
			else
			{
				m_currentInstanceScreenSize = std::numeric_limits<float>::max();
				m_mesh->UseWithScreenSize( m_currentScreenSize, m_worldBoundingSphere.radius );
			}
		}
		else
		{
			m_currentInstanceScreenSize = std::numeric_limits<float>::max();
			m_mesh->UseWithScreenSize( m_currentScreenSize, m_worldBoundingSphere.radius );
		}

		m_currentScreenSize *= invLodFactor;
		m_currentInstanceScreenSize *= invLodFactor;

		if( frustum.IsBoxVisible( m_worldBoundingBox ) )
		{
			m_isVisible = parentLod >= m_lowestLodVisible && m_currentScreenSize >= m_minScreenSize;
			m_instancesVisible = m_isVisible && m_currentInstanceScreenSize >= s_instanceScreenSizeThreshold;
		}
	}

	if( !m_attachments.empty() )
	{
		auto [bones, boneCount] = GetBoneTransforms();

		for( auto it = begin( m_attachments ); it != end( m_attachments ); ++it )
		{
			( *it )->UpdateVisibility( updateContext, m_worldTransform, bones, boneCount );
		}
	}

	if( m_isVisible )
	{
		for( auto it = m_decals.begin(); it != m_decals.end(); ++it )
		{
			if( m_animationUpdater && m_animationUpdater->IsInitialized() )
			{
				auto [bones, boneCount] = GetBoneTransforms();

				( *it )->SetBoneMatrix( bones, int( boneCount ) );
			}
			( *it )->UpdateVisibility( updateContext, &m_parentData );
		}

		if( updateContext.m_raytracingEnabled )
		{
			UpdateRtMesh();
			UpdateRtSkeleton();
		}
	}
}

void EveChildMesh::UpdateRtMesh()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( !renderContext.GetCaps().SupportsRaytracing() )
	{
		return;
	}

	if( !m_mesh )
	{
		return;
	}

	auto areas = m_mesh->GetAreas( TRIBATCHTYPE_OPAQUE );
	if( !areas->empty() )
	{
		auto rtMesh = m_mesh->GetOrCreateRtMesh();
		// todo: dbl check with using currScreenSize
		rtMesh->UpdateRtMesh( m_mesh->GetGeometryResource(), m_mesh->GetMeshIndex(), m_currentScreenSize );

		for( auto it = begin( *areas ); it != end( *areas ); ++it )
		{
			( *it )->GetOrCreateRtMeshArea();
		}
	}
}

void EveChildMesh::UpdateRtSkeleton()
{
	if( !m_animationUpdater || !m_animationUpdater->IsInitialized() )
	{
		return;
	}

	auto rtMesh = m_mesh->GetRtMesh();
	if( !rtMesh )
	{
		return;
	}
	
	auto meshIndex = m_mesh->GetMeshIndex();
	auto lod = m_mesh->GetGeometryResource()->GetMeshLod( meshIndex, m_currentScreenSize );
	if( !lod )
	{
		return;
	}
	bool hasSkinned = false;
	
	auto areas = m_mesh->GetAreas( TRIBATCHTYPE_OPAQUE );	
	for( auto it = begin( *areas ); it != end( *areas ); ++it )
	{
		if( lod->m_areas[std::max( 0, ( *it )->GetIndex() )].m_isSkinned )
		{
			hasSkinned = true;
			break;
		}
	}

	if( !hasSkinned )
	{
		return; //no skinned areas
	}

	auto [bones, boneCount] = GetBoneTransforms();

	m_boneOffsets.UploadTransforms( Tr2BoneTransformBuffer::GetInstance(), reinterpret_cast<const Tr2BoneTransformBuffer::Float4x3*>( bones ), uint32_t( boneCount ) );
	auto offset = m_boneOffsets.GetCurrentFrameOffset();

	bool skeletonChanged = rtMesh->SetBoneTransforms( boneCount, bones, offset );

	if( skeletonChanged )
	{
		//Skeleton has changed, so mark all area BLAS's as out-of-date.
		for( auto it = begin( *areas ); it != end( *areas ); ++it )
		{
			auto meshAreaIndex = std::max( 0, ( *it )->GetIndex() );
			if( lod->m_areas[meshAreaIndex].m_isSkinned )
			{
				( *it )->GetRtMeshArea()->MarkBlasOutdated();
			}
		}
	}
}

void EveChildMesh::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( m_isVisible )
	{
		if( m_instancedMesh ) 
		{
			if( m_instancesVisible )
			{
				renderables.push_back( this );
				if (!m_decals.empty())
				{
					auto geometryRes = m_mesh->GetGeometryResource();

					if (geometryRes)
					{
						DecalMeshCache meshCache;
						// run over every decal and update it
						for (EveSpaceObjectDecalVector::const_iterator it = m_decals.begin(); it != m_decals.end(); ++it)
						{
							// now prep to get the renderables
							( *it )->GetInstancedRenderables( renderables, meshCache, m_instancedMesh, m_currentInstanceScreenSize );
						}
					}
				}
			}
		}
		else
		{
			renderables.push_back( this );

			if( !m_decals.empty() )
			{
				if( auto geometryRes = m_mesh->GetGeometryResource() )
				{
					DecalMeshCache meshCache;
					// run over every decal and update it
					for( EveSpaceObjectDecalVector::const_iterator it = m_decals.begin(); it != m_decals.end(); ++it )
					{
						// now prep to get the renderables
						( *it )->GetRenderables( renderables, meshCache, geometryRes, m_currentScreenSize );
					}
				}
			}
		}
	}
}

bool EveChildMesh::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	if( m_worldBoundingSphere )
	{
		sphere = Vector4( m_worldBoundingSphere.center, m_worldBoundingSphere.radius );
		return true;
	}

	return false;
}

bool EveChildMesh::HasTransparentBatches()
{
	if( m_display && m_mesh )
	{
		return !(m_mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT )->empty());
	}

	return false;
}

bool EveChildMesh::IsVisible( const EveUpdateContext& updateContext ) const
{
	if( m_worldBoundingSphere.radius > 0 )
	{
		auto& frustum = updateContext.GetFrustum();
		if( frustum.IsSphereVisible( m_worldBoundingSphere.center, m_worldBoundingSphere.radius ) )
		{
			return frustum.GetPixelSizeAccrossEst( m_worldBoundingSphere.center, m_worldBoundingSphere.radius ) >= updateContext.GetVisibilityThreshold();
		}
	}
	return false;
}

void EveChildMesh::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	if( m_display )
	{
		if( m_mesh )
		{
			m_mesh->GetBatches( batches, m_mesh->GetAreas( batchType ), perObjectData, min( m_currentInstanceScreenSize, m_currentScreenSize ) );
		}
		
		if( m_activationStrength != 0.0 )
		{
			for( auto it = begin( m_attachments ); it != end( m_attachments ); ++it )
			{
				( *it )->GetBatches( batches, batchType, perObjectData, reason );
			}
		}	
	}
}

void EveChildMesh::GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData, float shadowPixelSize )
{
	// TODO: Figure out what we want to do with shadows
	// Fix asap <Logi 27. aug 2015>
	if( m_display && m_mesh )
	{
		m_mesh->GetBatches( batches, m_mesh->GetAreas( TRIBATCHTYPE_OPAQUE ), perObjectData, shadowPixelSize );
	}
}

void EveChildMesh::PushRtGeometry( Tr2RaytracingManager& rtManager ) const
{
	if( !m_display || !m_mesh || !m_castShadow )
	{
		return;
	}

	auto rtMesh = m_mesh->GetRtMesh();

	if ( !rtMesh || !rtMesh->IsGood() )
	{
		return;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();

	const Tr2MeshAreaVector* opaqueAreas = m_mesh->GetAreas( TRIBATCHTYPE_OPAQUE );

	if( m_instancedMesh )
	{
		if( !m_instancedMesh->GetDisplay() || m_instanceTransforms.empty() )
		{
			return;
		}

		m_instanceWorldTransforms.clear();
		m_instanceWorldTransforms.reserve( m_instanceTransforms.size() );

		for( const auto& instanceTransform : m_instanceTransforms )
		{
			auto m = XMMatrixMultiply( instanceTransform, m_worldTransform );
			m_instanceWorldTransforms.push_back( Tr2RaytracingGeometry::Float4x3( Matrix( m ) ) );
		}
		
		auto idm = IdentityMatrix();
		UpdateRtPerObjectData( m_psData, &idm, renderContext, m_rtPerObjectData );

		uint32_t vertexBufferDataIndex = 0;
		for( Tr2MeshAreaVector::const_iterator it = opaqueAreas->begin(); it != opaqueAreas->end(); ++it, ++vertexBufferDataIndex )
		{
			auto area = *it;
			if( area->GetDisplay() && area->IsCastingShadows() )
			{
				auto geometry = area->GetRtMeshArea();
				if( geometry )
				{
					const Tr2ConstantBufferAL* vertexBufferData = nullptr;
					if( area->HasVertexBufferAccessInRtShadow() )
					{
						vertexBufferData = area->GetRtMeshArea()->GetGeometryConstants( *rtMesh, renderContext );
					}
					rtManager.GetGeometry().AddGeometry( *rtMesh, *geometry, area->GetMaterialInterface(), &m_rtPerObjectData, vertexBufferData, m_instanceWorldTransforms.data(), m_instanceWorldTransforms.size() );
				}
			}
		}
	}
	else
	{
		UpdateRtPerObjectData( m_psData, nullptr, renderContext, m_rtPerObjectData );

		uint32_t vertexBufferDataIndex = 0;
		for( Tr2MeshAreaVector::const_iterator it = opaqueAreas->begin(); it != opaqueAreas->end(); ++it, ++vertexBufferDataIndex )
		{
			auto area = *it;
			if( area->GetDisplay() && area->IsCastingShadows() )
			{
				auto geometry = area->GetRtMeshArea();
				if( geometry )
				{
					const Tr2ConstantBufferAL* vertexBufferData = nullptr;
					if( area->HasVertexBufferAccessInRtShadow() )
					{
						vertexBufferData = area->GetRtMeshArea()->GetGeometryConstants( *rtMesh, renderContext );
					}
					rtManager.GetGeometry().AddGeometry( *rtMesh, *geometry, area->GetMaterialInterface(), &m_rtPerObjectData, vertexBufferData, m_worldTransform );
				}
			}
		}
	}

	rtManager.GetGeometry().AddBindlessResources( *opaqueAreas, *rtMesh );
}

void EveChildMesh::SetInstanceTransforms( std::vector<Matrix> instances )
{
	m_instanceTransforms = instances;
}

float EveChildMesh::GetSortValue()
{
	Vector3 d = Tr2Renderer::GetViewPosition() - m_worldTransform.GetTranslation();
	float distance = Length( d );
	return distance * m_sortValueScale + m_sortValueOffset;
}

Tr2PerObjectData* EveChildMesh::GetShadowPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	return GetPerObjectData( accumulator );
}

Tr2PerObjectData* EveChildMesh::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	if( m_animationUpdater && m_animationUpdater->IsInitialized() )
	{
		auto [bones, boneCount] = GetBoneTransforms();
		m_vsData.boneOffsets[2] = uint32_t( boneCount );
		m_boneOffsets.UploadTransforms( Tr2BoneTransformBuffer::GetInstance(), reinterpret_cast<const Tr2BoneTransformBuffer::Float4x3*>( bones ), uint32_t( boneCount ) );
	}
	m_vsData.boneOffsets[0] = m_boneOffsets.GetCurrentFrameOffset();
	m_vsData.boneOffsets[1] = m_boneOffsets.GetPreviousFrameOffset();

	Tr2PerObjectDataWithPersistentBuffers<EveChildMesh>* perObjectData = accumulator->Allocate<Tr2PerObjectDataWithPersistentBuffers<EveChildMesh>>();
	if( !perObjectData )
	{
		return nullptr;
	}
	perObjectData->Initialize( this, &m_perObjectDataVs, &m_perObjectDataPs );

	return perObjectData;
}

uint32_t EveChildMesh::GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const
{
	if( shaderType == Tr2RenderContextEnum::PIXEL_SHADER )
	{
		return sizeof( m_psData );
	}
	else
	{
		return sizeof( m_vsData );
	}
}

IRoot* EveChildMesh::GetID( uint16_t )
{
	return GetRawRoot();
}

void EveChildMesh::GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData )
{
	if( ( pickTypes & PICK_TYPE_OPAQUE ) != 0 )
	{
		GetBatches( batches, TRIBATCHTYPE_OPAQUE, perObjectData );
		GetBatches( batches, TRIBATCHTYPE_DECAL, perObjectData );
	}
	if( ( pickTypes & PICK_TYPE_TRANSPARENT ) != 0 )
	{
		if( !m_mesh || !m_mesh->GetDisplay() )
		{
			return;
		}

		if( auto areas = m_mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT ) )
		{
			m_mesh->GetBatches( batches, areas, perObjectData );
		}
		if( auto areas = m_mesh->GetAreas( TRIBATCHTYPE_ADDITIVE ) )
		{
			m_mesh->GetBatches( batches, areas, perObjectData );
		}
	}
}

void EveChildMesh::UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* data )
{
	if( shaderType == Tr2RenderContextEnum::PIXEL_SHADER )
	{
		memcpy( data, &m_psData, sizeof( m_psData ) );
	}
	else
	{
		memcpy( data, &m_vsData, sizeof( m_vsData ) );
	}
}

void EveChildMesh::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	m_boneOffsets.AdvanceFrame();

	m_perObjectDataVs.InvalidateBufferData();
	m_perObjectDataPs.InvalidateBufferData();

	Matrix localToWorldTransform = params.localToWorldTransform;
	Matrix lastWorldTransform = m_worldTransform;

	UpdateTransform( localToWorldTransform );
	for( auto it = m_transformModifiers.begin(); it != m_transformModifiers.end(); it++ )
	{
		m_worldTransform = (*it)->ApplyTransform( m_worldTransform, params.boneCount, params.bones );
	}

	// need to update the data we get from the parent to be relevant to us!
	if( nullptr != params.spaceObjectParent )
	{
		params.spaceObjectParent->GetPerObjectStructs( m_vsData, m_psData );
		params.spaceObjectParent->GetParentData( &m_parentData );

		// need to move the clipdata inversely of the translation of the childmesh
		m_vsData.clipData = Vector4( m_vsData.clipData.GetXYZ() - m_translation, m_vsData.clipData.w );
		m_psData.clipSphereCenter = m_psData.clipSphereCenter - m_translation;
		
		// update the world transform of the parent
		m_parentData.transform = m_worldTransform;
	}

	m_activationStrength = params.activationStrength;

	m_vsData.worldTransform = Transpose( m_worldTransform );
	m_vsData.invWorldTransform = Inverse( m_vsData.worldTransform );
	m_vsData.worldTransformLast = Transpose( lastWorldTransform );

	m_psData.worldTransform = m_vsData.worldTransform;
	m_psData.worldTransformLast = m_vsData.worldTransformLast;
	m_psData.invWorldTransform = m_vsData.invWorldTransform;

	// Normalize screenSize dimensions
	auto screen_width = Tr2Renderer::GetViewport().width;
	auto screen_height = Tr2Renderer::GetViewport().height;
	m_psData.screenSize.x = min( float( m_currentScreenSize / screen_width ), 1.0f );
	m_psData.screenSize.y = min( float( m_currentScreenSize / screen_height ), 1.0f );

	if( !m_attachments.empty() )
	{
		auto [bones, boneCount] = GetBoneTransforms();
		for( auto& attachment : m_attachments )
		{
			// If we need boostergain then we will need to get it from the parent (the 0.0 in the param list)
			attachment->UpdateLights( m_worldTransform, bones, boneCount, m_activationStrength, 0.0 );
		}
	}

	if( m_mesh )
	{
		m_worldBoundingBox = m_mesh->GetBounds();
		if( m_worldBoundingBox )
		{
			m_worldBoundingBox.Transform( m_worldTransform );
			m_worldBoundingSphere = CcpMath::Sphere( m_worldBoundingBox );
		}
		else
		{
			m_worldBoundingSphere = {};
		}
	}
	else
	{
		m_worldBoundingBox = {};
		m_worldBoundingSphere = {};
	}
}

void EveChildMesh::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& )
{
	if( m_animationUpdater )
	{
		if( m_mesh && m_animationUpdater->GetResPath().empty() )
		{
			if( m_mesh->GetGeometryResource() != m_animationUpdater->GetSharedGeometryRes() )
			{
				InitializeAnimation();
			}
		}

		if( m_updateAnimation )
		{
			m_animationUpdater->PrePhysicsAnimation( 0, IdentityMatrix() );
		}

		if( m_mesh && !m_animationUpdater->m_meshBinding )
		{
			if( !m_meshBinding || m_meshBinding->GetAnimation() != m_animationUpdater || m_meshBinding->GetGeometryRes() != m_mesh->GetGeometryResource() || m_meshBinding->GetMeshIndex() != m_mesh->GetMeshIndex() )
			{
				m_meshBinding = std::make_unique<Tr2AnimationMeshBinding>( m_animationUpdater, m_mesh->GetGeometryResource(), m_mesh->GetMeshIndex() );
			}
		}
		else
		{
			m_meshBinding = {};
		}
	}
	else
	{
		m_meshBinding = {};
	}
}

void EveChildMesh::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

void EveChildMesh::ChangeLOD( Tr2Lod )
{
}

void EveChildMesh::SetMesh( Tr2MeshBase* mesh )
{
	m_mesh = mesh;
	m_instancedMesh = BlueCastPtr( m_mesh );
}

void EveChildMesh::SetOrigin( Origin origin )
{
	m_origin = origin;
}

// --------------------------------------------------------------------------------
// Description:
//   Setup function to set data from outside, in this case just pass it to
//   function of base class
// --------------------------------------------------------------------------------
void EveChildMesh::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	// call base class's setup
	EveChildTransform::Setup( scale, rotation, translation, lowestLodVisible );

	// and remember lodding!
	m_lowestLodVisible = lowestLodVisible;
}

bool EveChildMesh::IsAlwaysOn() const
{
	return true;
}

void EveChildMesh::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	if ( nullptr != m_mesh )
	{
		m_mesh->SetShaderOption( name, value );
	}

	for( EveSpaceObjectDecalVector::iterator it = m_decals.begin(); it != m_decals.end(); ++it )
	{
		( *it )->SetShaderOption( name, value );
	}

	for( auto it = m_attachments.begin(); it != m_attachments.end(); ++it )
	{
		IEveSpaceObjectAttachment* attachment = *it;
		attachment->SetShaderOption( name, value );
	}
}

void EveChildMesh::SetScale( const Vector3& scale )
{
	m_scaling = scale;
}

void EveChildMesh::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	if( m_mesh )
	{
		m_mesh->GetDebugOptions( options );
	}
	options.insert( "Bones" );
	options.insert( "Decals" );
	options.insert( "Lights" );

	for( auto it = begin( m_attachments ); it != end( m_attachments ); ++it )
	{
		( *it )->GetDebugOptions( options );
	}
}

void EveChildMesh::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( !m_display )
	{
		return;
	}
	if( m_mesh )
	{
		m_mesh->RenderDebugInfo( m_worldTransform, renderer );
	}
	if( m_animationUpdater && renderer.HasOption( GetRawRoot(), "Bones" ) )
	{
		m_animationUpdater->RenderBones( m_worldTransform, m_meshBinding.get() );
	}

	if( renderer.HasOption( GetRawRoot(), "Decals" ) )
	{
		for( EveSpaceObjectDecalVector::iterator it = m_decals.begin(); it != m_decals.end(); ++it )
		{
			( *it )->RenderDebugInfo( renderer, m_worldTransform );
		}
	}

	if( !m_attachments.empty() )
	{
		auto [bones, boneCount] = GetBoneTransforms();

		for( auto it = begin( m_attachments ); it != end( m_attachments ); ++it )
		{
			( *it )->RenderDebugInfo( renderer, m_worldTransform, bones, boneCount );
		}
	}

	if( renderer.HasOption( GetRawRoot(), "Lights" ) )
	{
		auto [bones, boneCount] = GetBoneTransforms();
		for( auto it = m_lights.begin(); it != m_lights.end(); ++it )
		{
			( *it )->RenderDebugInfo( renderer, m_worldTransform, bones, boneCount );
		}
	}
}

void EveChildMesh::AddTransformModifier( IEveChildTransformModifier* modifier )
{
	m_transformModifiers.Append( modifier );
}

Tr2GrannyAnimation* EveChildMesh::GetAnimationController() const
{
	return m_animationUpdater;
}

void EveChildMesh::SetAnimationController( Tr2GrannyAnimation* animation )
{
	m_animationUpdater = animation;
	InitializeAnimation();
}

void EveChildMesh::AddDecal( EveSpaceObjectDecalPtr newDecal )
{
	newDecal->SetPriority( (uint32_t)m_decals.size() );
	m_decals.Insert( -1, newDecal->GetRawRoot() );
}

void EveChildMesh::AddAttachment( IEveSpaceObjectAttachment* attachment )
{
	m_attachments.Append( attachment );
}

void EveChildMesh::ClearAttachments()
{
	m_attachments.Clear();
}

void EveChildMesh::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	for( auto it = begin( m_attachments ); it != end( m_attachments ); ++it )
	{
		( *it )->RegisterWithQuadRenderer( quadRenderer );
	}
}

std::pair<const granny_matrix_3x4*, size_t> EveChildMesh::GetBoneTransforms() const
{
	size_t boneCount = 0;
	const granny_matrix_3x4* bones = nullptr;

	if( !m_animationUpdater || !m_animationUpdater->IsInitialized() )
	{
		return std::make_pair( nullptr, 0 );
	}

	auto accumulatedTransforms = m_animationUpdater->GetAnimationTransforms();
	if( m_animationUpdater->m_meshBinding )
	{
		Tr2GrannyAnimationUtils::GetBoneList( m_animationUpdater, bones, boneCount );
		return std::make_pair( bones, boneCount );
	}
	if( m_meshBinding )
	{
		return m_meshBinding->GetBoneTransforms();
	}
	return std::make_pair( nullptr, 0 );
}
	
void EveChildMesh::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	if( m_attachments.empty() || !m_isVisible || !m_display )
	{
		return;
	}
	auto [bones, boneCount] = GetBoneTransforms();
	for( auto it = begin( m_attachments ); it != end( m_attachments ); ++it )
	{
		// If we need boostergain then we will need to get it from the parent (the 0.0 in the param list)
		( *it )->AddToQuadRenderer( quadRenderer, m_worldTransform, m_activationStrength, 0.0, bones, boneCount );
	}
}

void EveChildMesh::GetLights( Tr2LightManager& lightManager ) const
{
	if( m_lights.empty() || !m_display )
	{
		return;
	}

	auto [bones, boneCount] = GetBoneTransforms();

	for( auto it = std::begin( m_lights ); it != std::end( m_lights ); ++it )
	{
		( *it )->AddLight( lightManager, m_worldTransform, 1.0f, bones, boneCount );
		( *it )->SetBrightnessMultiplier( m_activationStrength );
	}
}

void EveChildMesh::AddLight( Tr2Light* newLight )
{
	m_lights.Append( newLight->GetRawRoot() );
}

void EveChildMesh::ClearLights()
{
	m_lights.Clear();
}

void EveChildMesh::SetReflectionMode( EntityComponents::ReflectionMode mode )
{
	m_reflectionMode = mode;
	if( GetComponentRegistry() )
	{
		ReRegister();
	}
}

void EveChildMesh::SetCastShadow( bool castShadow )
{
	m_castShadow = castShadow;
}

void EveChildMesh::SetMinScreenSize( float minScreenSize )
{
	m_minScreenSize = minScreenSize;
}

BluePy EveChildMesh::GetSofSourceLocator( uint32_t areaId ) const
{
	IWeakObjectPtr weak = BlueCastPtr( GetRawRoot() );
	if( auto metadata = BeObjectMetadata->GetMetadata( weak ) )
	{
		auto hull = metadata->find( "SofParentHullName" );
		if( hull == metadata->end() )
		{
			return BluePy( Py_None, true );
		}
		auto locatorSet = metadata->find( "SofLocatorSetName" );
		if( locatorSet == metadata->end() )
		{
			return BluePy( Py_None, true );
		}
		uint32_t locatorIndex = 0;
		if( m_instancedMesh )
		{
			locatorIndex = areaId;
		}
		else
		{
			auto locatorIndexStr = metadata->find( "SofLocatorIndex" );
			if( locatorIndexStr != metadata->end() )
			{
				locatorIndex = (uint32_t)strtoul( locatorIndexStr->second.c_str(), nullptr, 10 );
			}
		}
		BluePy result = BluePy( PyTuple_New( 3 ) );
		PyTuple_SetItem( result, 0, ToPython( hull->second.c_str() ) );
		PyTuple_SetItem( result, 1, ToPython( locatorSet->second.c_str() ) );
		PyTuple_SetItem( result, 2, ToPython( locatorIndex ) );
		return result;
	}
	return BluePy( Py_None, true );
}