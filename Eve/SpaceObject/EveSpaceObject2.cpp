#include "StdAfx.h"

#include "Utilities/BoundingBox.h"
#include "Utilities/BoundingSphere.h"
#include "Utilities/ViewDistanceInfo.h"

#include "include/ITr2DebugRenderer.h"
#include "include/IEveBallpark.h"
#include "Include/TriMath.h"
#include "Resources/TriGeometryRes.h"
#include "TriFrustumOrtho.h"

#include "Eve/EveTransform.h"
#include "EveSpaceObject2.h"
#include "Attachments/EveSpaceObjectDecal.h"
#include "Eve/EveSpaceScene.h"
#include "Utils/EveLocator2.h"
#include "Attachments/EveSpriteSet.h"
#include "Attachments/EveSpotlightSet.h"
#include "Attachments/EvePlaneSet.h"
#include "Tr2GPUParticleEmitter.h"
#include "Tr2MeshLod.h"
#include "Tr2GrannyAnimation.h"
#include "Tr2BindingVector3.h"
#include "Eve/EveUpdateContext.h"
#include "Eve/Animation/EveAnimationSequencer.h"
#include "Tr2Effect.h"
#include "Utils/EveCustomMask.h"
#include "TriSettingsRegistrar.h"

#include <limits>

BLUE_DEFINE_INTERFACE( IBlueObjectProxy );

static const int MAX_JOINT_COUNT = 58;

static const double UNINITIALIZED_POSITION = std::numeric_limits<double>::infinity();

CCP_STATS_DECLARE( eveLowDetailObjects, "Trinity/EveSpaceObject2/lowDetailObjects", true, CST_COUNTER_LOW, "Number of objects rendered in low detail per frame.");
CCP_STATS_DECLARE( eveHighDetailObjects, "Trinity/EveSpaceObject2/highDetailObjects", true, CST_COUNTER_LOW, "Number of objects rendered in high detail per frame.");

float g_secondaryLightingRadiusCutoffFactor = 0.3;
TRI_REGISTER_SETTING( "secondaryLightingRadiusCutoffFactor", g_secondaryLightingRadiusCutoffFactor );

static BlueStructureDefinition EveDamageLocatorStructureDef[] =
{ 
	{ "position", Be::FLOAT32_3, 0 }, 
	{ "impactDirection", Be::FLOAT32_4, 12 }, 
	{0} 
};


// --------------------------------------------------------------------------------
// Description:
//   Copy all the matrices to HW
// --------------------------------------------------------------------------------
void EveSpaceObjectPerObjectData::SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
{
	Tr2PerObjectDataWithPersistentBuffers<EveSpaceObject2>::SetPerObjectDataToDevice( buffers, constantTypeMask, renderContext );
	renderContext.SetNumberOfLights( m_psPointLightCount );
}

// --------------------------------------------------------------------------------
// Description:
//   The base class for most of the objects in a space scene
// --------------------------------------------------------------------------------
EveSpaceObject2::EveSpaceObject2( IRoot* lockobj ) :
	PARENTLOCK( m_persistedDamageLocators ),
	PARENTLOCK( m_decals ),
	PARENTLOCK( m_locators ),
	PARENTLOCK( m_observers ),
	PARENTLOCK( m_spriteSets ),
	PARENTLOCK( m_spotlightSets ),
	PARENTLOCK( m_planeSets ),
	PARENTLOCK( m_children ),
	PARENTLOCK( m_curveSets ),
	PARENTLOCK( m_overlayEffects ),
	PARENTLOCK( m_particleEmittersGPU ),
	m_display( true ),
	m_update( true ),
	m_enableShadow( true ),
	m_allowLodSelection( false ),
	m_estimatedPixelDiameter( 0.0f ),
	m_estimatedPixelDiameterWithChildren( 0.0f ),
	m_boundingSphereCenter( 0.f, 0.f, 0.f ),
	m_boundingSphereRadius( -1.f ),
	m_boundingSphereWorld( 0.f, 0.f, 0.f, -1.f ),
	m_dynamicBoundingSphere( 0.f, 0.f, 0.f, -1.f ),
	m_albedoColor( 0.f, 0.f, 0.f, 1.f ),
	m_secondaryLightingSphereRadius( 0.f ),
	m_modelScale( 1.f ),
	m_lodLevel( TR2_LOD_UNSPECIFIED ),
	m_lodLevelWithChildren( TR2_LOD_UNSPECIFIED ),
	m_debugShowBoundingBox( true ),
	m_debugShowMeshAreaBoundingBox( false ),
	m_debugRenderDebugInfoForChildren( true ),
	m_debugShowDynamicBounds( true ),
	m_isVisible( false ),
	m_isMeshVisible( false ),
	m_localAabbMin( 0,0,0 ),
	m_localAabbMax( 0,0,0 ),
	m_lastCurveUpdateTime( 0 ),
	m_previousPosition( UNINITIALIZED_POSITION, UNINITIALIZED_POSITION, UNINITIALIZED_POSITION ),
	m_spaceObjectMiscData( 1.f, 1.f, EVE_SPACEOBJECT_DIRT_LEVEL_DEFAULT, 1.f ),
	m_spaceObjectClipData( 0.f, 0.f, 0.f, 0.f ),
	m_spaceObjectClipDataEx( 0.f, 0.f, 0.f, 0.f ),
	m_psPointLightCount( 0 ),
	m_displayChildren( true ),
	m_isAnimated( false )
{
	m_positionDelta.CreateInstance();

	m_decalCache = CCP_NEW( "EveSpaceObject2::m_decalCache" ) EveSpaceObjectDecalCache;
	m_damageLocatorsUpdatedThisFrame = false;
	m_impactDirectionsUpdatedThisFrame = false;
	m_allocatedDamageLocatorCount = 0;
	m_transformedDamageLocators = NULL;
	m_damageLocatorPositions = NULL;
	m_transformedImpactDirections = NULL;
	m_alignedTransformMatrix = (XMMATRIX*)CCP_ALIGNED_MALLOC( "EveSpaceObject2/m_alignedTransformMatrix", sizeof(XMMATRIX), 16);

	m_animationUpdater.CreateInstance();
	m_persistedDamageLocators.SetStructureDefinition( EveDamageLocatorStructureDef );
	memset( m_shLightingCoefficients, 0, sizeof( m_shLightingCoefficients ) );
}

EveSpaceObject2::~EveSpaceObject2()
{
	FreeAnimationData();

	if( m_geometryResFromMesh )
	{
		m_geometryResFromMesh->RemoveNotifyTarget( this );
	}

	if( m_transformedDamageLocators )
	{
		CCP_ALIGNED_FREE( m_transformedDamageLocators );
	}

	if( m_transformedImpactDirections )
	{
		CCP_ALIGNED_FREE( m_transformedImpactDirections );
	}

	if( m_damageLocatorPositions )
	{
		CCP_ALIGNED_FREE( m_damageLocatorPositions );
	}

	CCP_ALIGNED_FREE( m_alignedTransformMatrix );
	CCP_DELETE m_decalCache;
}

bool EveSpaceObject2::Initialize()
{
	RebuildDamageLocatorCache();

	// Disallow LOD selection until it's been established we have LODs.
	m_allowLodSelection = false;

	if( m_meshLod )
	{
		if( m_meshLod->GetSelectedLod() == TR2_LOD_UNSPECIFIED )
		{
			m_meshLod->SelectLod( TR2_LOD_LOW );
		}
		m_allowLodSelection = true;
	}

	if( m_mesh )
	{
		PrepareForAnimation();
	}

	return true;
}

void EveSpaceObject2::RegisterSecondaryLightSource( Tr2ShLightingManager& manager )
{
	static const Color s_noEmissiveColor( 0.f, 0.f, 0.f, 0.f );
	manager.RegisterSecondaryLightSource( &m_worldTransform.GetTranslation(), &m_secondaryLightingSphereRadius, &m_albedoColor, &s_noEmissiveColor );
}

void EveSpaceObject2::UnregisterSecondaryLightSource( Tr2ShLightingManager& manager )
{
	manager.UnregisterSecondaryLightSource( &m_worldTransform.GetTranslation() );
}

void EveSpaceObject2::UpdateSyncronous( EveUpdateContext& updateContext )
{
	Be::Time time = updateContext.GetTime();

	UpdateWorldTransform( time );

	if( !m_update )
	{
		return;
	}
		
	if( m_allowLodSelection )
	{
		UnloadLodIfNeeded( time );
	}

	// Reset this so we do a transformed damage locator update if needed
	m_damageLocatorsUpdatedThisFrame = false;
	m_impactDirectionsUpdatedThisFrame = false;

	// Particle Systems
	// Get the reference position
	Vector3d referencePosition( 0.0, 0.0, 0.0 );
	IEveReferencePointPtr refObject( BlueCastPtr( m_ballPosition ) );
	if( refObject )
	{
		refObject->GetReferencePoint( &referencePosition, time );
	}

	if( m_previousPosition.x != UNINITIALIZED_POSITION )
	{
		Vector3 positionDelta(
			float( referencePosition.x - m_previousPosition.x ),
			float( referencePosition.y - m_previousPosition.y ),
			float( referencePosition.z - m_previousPosition.z ) );
		Matrix worldInverse;
		D3DXMatrixInverse( &worldInverse, nullptr, &m_worldTransform );
		D3DXVec3TransformNormal( &m_positionDelta->m_value, &positionDelta, &worldInverse );
	}
	else
	{
		m_positionDelta->m_value = Vector3( 0.f, 0.f, 0.f );
	}
	m_previousPosition = referencePosition;
	
	//update GPU emitters (these only need an egoball-relative position)
	if( !m_particleEmittersGPU.empty() ) 
	{
		Matrix transformWithoutTranslation = m_worldTransform;
		transformWithoutTranslation._41 -= m_worldPosition.x;
		transformWithoutTranslation._42 -= m_worldPosition.y;
		transformWithoutTranslation._43 -= m_worldPosition.z;

		Tr2GPUParticlePoolManager* manager = updateContext.GetParticlePoolManager();
		if( manager != NULL )
		{
			Vector3 relativePosition(0,0,0), relativeVelocity(0,0,0);
			if( m_ballPosition ) {
				m_ballPosition->GetValueAt( &relativePosition, time );
				m_ballPosition->GetValueDotAt( &relativeVelocity, time );
			}
			for( auto it = m_particleEmittersGPU.begin(); it != m_particleEmittersGPU.end(); ++it )
			{
				(*it)->ApplyPool( manager );
				(*it)->UpdateTransform( relativePosition, relativeVelocity, manager->GetLastEgoTranslation(), transformWithoutTranslation );
			}
		}
	}

	//
	// Animation
	//
	if( m_animationSequencer )
	{
		m_animationSequencer->Update( time );
	}
	if( m_animationUpdater )
	{
		Matrix m;
		D3DXMatrixIdentity( &m );
		m_animationUpdater->PrePhysicsAnimation( 0, m );
	}

	TriObserverLocalVector::iterator observersEnd = m_observers.end();
	for( TriObserverLocalVector::iterator it = m_observers.begin(); it != observersEnd; ++it )
	{
		(*it)->Update( m_worldTransform );
	}

	Tr2MeshBase* mesh = m_meshLod;
	if( m_mesh )
	{
		mesh = m_mesh;
	}
	if( m_secondaryLightingSphereRadius <= 0 && mesh && mesh->GetGeometryResource() && mesh->GetGeometryResource()->IsGood() )
	{
		// to approximate space object as a secondary light emitter we take a sphere of the same volume as its bounding box
		Vector3 aabbMin, aabbMax;
		if( mesh->GetGeometryResource()->GetBoundingBox( mesh->GetMeshIndex(), aabbMin, aabbMax ) )
		{
			Vector3 aabbSize = aabbMax - aabbMin;
			float boxVolume = aabbSize.x * aabbSize.y * aabbSize.z;
			m_secondaryLightingSphereRadius = pow( boxVolume / 4.f * 3.f / TRI_PI, 1.0f / 3.0f );
		}
	}
}

void EveSpaceObject2::UpdateAsyncronous( EveUpdateContext& updateContext )
{
	if( !m_update )
	{
		return;
	}

	Be::Time time = updateContext.GetTime();

	m_perObjectDataVs.InvalidateBufferData();
	m_perObjectDataPs.InvalidateBufferData();

	// prepare shader data: shader needs to know size of this object for some surface-scaling issues
	m_spaceObjectMiscData.w = GetBoundingSphereRadius();

	if( m_isAnimated && m_animationUpdater && m_animationUpdater->IsInitialized() )
	{
		m_animationUpdater->GetDynamicBounds( m_dynamicBoundingSphere, m_localAabbMin, m_localAabbMax );
		D3DXVec3TransformCoord( (Vector3*)&m_boundingSphereWorld, (Vector3*)&m_dynamicBoundingSphere, &m_worldTransform );
		m_boundingSphereWorld.w = m_modelScale * m_dynamicBoundingSphere.w;
	}
	else if( m_boundingSphereRadius > 0.0f )
	{
		D3DXVec3TransformCoord( (Vector3*)&m_boundingSphereWorld, &m_boundingSphereCenter, &m_worldTransform );
		m_boundingSphereWorld.w = m_modelScale * m_boundingSphereRadius;
	}

	if( !m_curveSets.empty() || !m_overlayEffects.empty() )
	{
		float delta = (float)TimeAsDouble( time - m_lastCurveUpdateTime );

		if( EveLODHelper::ShouldUpdate( m_lodLevelWithChildren, delta ) )
		{
			m_lastCurveUpdateTime = time;
			for( TriCurveSetVector::const_iterator it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
			{
				(*it)->Update( time, time );
			}

			for( auto it = m_overlayEffects.begin(); it != m_overlayEffects.end(); ++it )
			{
				(*it)->Update( time, time );
			}
		}
	}

	for( IEveTransformVector::const_iterator it = m_children.begin(); it != m_children.end(); ++it )
	{
		(*it)->Update( updateContext );
	}
}

void EveSpaceObject2::RenderDebugInfo( Tr2RenderContext& renderContext )
{
	// debug info of custom maps stops all other debug info.
	if( m_customMask )
	{
		Matrix customMaskTransform;
		m_customMask->GetDebugDrawMatrix( &customMaskTransform, GetBoundingSphereRadius() );
		Tr2Renderer::DrawOrientedBox( customMaskTransform, 0xff00ffff );
		return;
	}

	if( m_debugShowBoundingBox )
	{
		Vector3 minBounds( -0.5f, -0.5f, -0.5f );
		Vector3 maxBounds( 0.5f, 0.5f, 0.5f );
		uint32_t color = 0xff0000ff;

		if( m_mesh )
		{
			if( m_mesh->GetBoundingBox( minBounds, maxBounds ) )
			{
				color = 0xffffffff;
			}
		}

		BoundingBoxTransform( minBounds, maxBounds, m_worldTransform );
		Tr2Renderer::DrawBox( minBounds, maxBounds, color );

		Vector3 center;
		D3DXVec3TransformCoord( &center, &m_boundingSphereCenter, &m_worldTransform );
		Tr2Renderer::DrawSphere( center, m_boundingSphereRadius, 8, 0xffff00ff );
	}

	if( m_debugShowMeshAreaBoundingBox )
	{
		if( m_geometryResFromMesh )
		{
			for( unsigned int a = 0; a < m_geometryResFromMesh->GetAreaCount( 0 ); ++a )
			{
				Vector3 minBounds, maxBounds;
				if( m_mesh->GetAreaBoundingBox( a, minBounds, maxBounds ) )
				{
					BoundingBoxTransform( minBounds, maxBounds, m_worldTransform );
					Tr2Renderer::DrawBox( minBounds, maxBounds, 0xff00ffff );
				}
			}
		}
	}
	if( m_animationUpdater && m_debugShowDynamicBounds && m_isAnimated )
	{
		m_animationUpdater->RenderDynamicBounds( m_worldTransform );
	}
	Tr2Renderer::Printf( TRI_DBG_FONT_SMALL, m_worldTransform.GetTranslation(), 0xffffffff, m_name.c_str() );

	if( m_debugRenderDebugInfoForChildren )
	{
		for( IEveTransformVector::const_iterator it = m_children.begin(); it != m_children.end(); ++it )
		{
			(*it)->RenderDebugInfo( renderContext );
		}

		// are decals visible?
		if( DisplayDecals() )
		{
			for( EveSpaceObjectDecalVector::iterator it = m_decals.begin(); it != m_decals.end(); ++it )
			{
				(*it)->RenderDebugInfo( &m_worldTransform );
			}
		}
	}
}

bool EveSpaceObject2::HasTransparentBatches()
{
	if( !m_mesh )
	{
		return false;
	}
	
	if( !m_mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT )->empty() )
	{
		return true;
	}

	for( auto it = m_overlayEffects.begin(); it != m_overlayEffects.end(); ++it )
	{
		if( (*it)->HasTransparentArea() )
		{
			return true;
		}
	}

	return false;
	
}

void EveSpaceObject2::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData )
{
	if( !m_mesh )
	{
		return;
	}

	if( m_mesh->IsHidden() )
	{
		return;
	}

	// Implement special behavior for specific batch types on a spaceobject
	if( batchType == TRIBATCHTYPE_ADDITIVE )
	{
		for( auto it = m_spriteSets.begin(); it != m_spriteSets.end(); ++it )
		{
			(*it)->GetBatches( batches, perObjectData );
		}

		for (auto it = m_spotlightSets.begin(); it != m_spotlightSets.end(); ++it )
		{
			(*it)->GetBatches( batches, perObjectData );
		}

		for (auto it = m_planeSets.begin(); it != m_planeSets.end(); ++it )
		{
			(*it)->GetBatches( batches, perObjectData );
		}
	}

	// Everything except for shadow batches
	Tr2MeshAreaVector* areas = m_mesh->GetAreas( batchType );
	// Could be NULL if we're rendering a batch type that hasn't got a mesh area vector
	if( areas )
	{
		// transparent needs sorted meshareas
		if( batchType != TRIBATCHTYPE_TRANSPARENT )
		{
			m_mesh->GetBatches( batches, areas, perObjectData );
		}
		else
		{
			GetSortedBatchesFromMeshAreaVector( areas, batches, perObjectData );
		}
	}

	// add overlay effect batches
	GetBatchesFromOverlayVector( batches, perObjectData, batchType );
}


void EveSpaceObject2::GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData )
{
	if( !m_shadowEffect )
	{
		return;
	}

	Tr2MeshBase* mesh = NULL;
	if( m_mesh )
	{
		mesh = m_mesh;
	}

	if( !mesh || mesh->IsHidden() )
	{
		return;
	}

	Tr2MeshAreaVector* areas = mesh->GetAreas( TRIBATCHTYPE_OPAQUE );

	TriGeometryRes* geomRes = mesh->GetGeometryResource();
	int meshIx = mesh->GetMeshIndex();

	for( Tr2MeshAreaVector::iterator it = areas->begin(); it != areas->end(); ++it )
	{
		Tr2MeshArea* area = *it;
		ITr2ShaderMaterial* material = area->GetMaterialInterface();

		if( area->IsHidden() || !material )
		{
			continue;
		}
		TriGeometryBatch* batch = batches->Allocate<TriGeometryBatch>();
		// Note that this can fail if the accumulator can't add more batches!
		if( batch )
		{
			batch->SetShaderMaterial( m_shadowEffect );
			batch->SetPerObjectData( perObjectData );
			batch->SetGeometryResource( geomRes );
			batch->SetMeshParameters( meshIx, area->GetIndex(), area->GetCount() );

			batches->Commit( batch );
		}
	}
}

float EveSpaceObject2::GetSortValue()
{
	Vector3 d = Tr2Renderer::GetViewPosition() - m_worldTransform.GetTranslation();
	float distance = D3DXVec3Length( &d );
	return distance;
}


// ---------------------------------------------------------------------------------------
//  Description:
//    Given a pointer to a mesh area vector, gathers TriGeometryBatches for each of the
//    areas. Order of batches is sorted, based upon distance to camera.
// Arguments:
//   areas - mesharea vector to collect from
//   batches - accumulator for the new batches
//   perObjectData - the per-object data for these batches
// ---------------------------------------------------------------------------------------
void EveSpaceObject2::GetSortedBatchesFromMeshAreaVector( const Tr2MeshAreaVector* areas, 
														  ITriRenderBatchAccumulator* batches, 
														  const Tr2PerObjectData* perObjectData ) const
{
	TriGeometryRes* geomRes = NULL;

	if( !geomRes )
	{
		geomRes = m_mesh->GetGeometryResource();
	}

	if( !geomRes || !geomRes->IsGood() )
	{
		return;
	}

	int meshIx = m_mesh->GetMeshIndex();

	// build a sortable mesharea item list
	Tr2MeshAreaItemList meshAreasToSort("EveSpaceObject2/SortMeshAreaVector");
	meshAreasToSort.reserve( areas->size() );
	for( Tr2MeshAreaVector::const_iterator it = areas->begin(); it != areas->end(); ++it )
	{
		Tr2MeshAreaPtr meshArea = *it;
		if( !meshArea->IsHidden() )
		{
			// get center of this area in object-space
			Vector3 minBBox, maxBBox, centerBBox( 0.f, 0.f, 0.f );
			if( geomRes->GetAreaBoundingBox( meshIx, meshArea->GetIndex(), minBBox, maxBBox ) )
			{
				centerBBox = 0.5f * ( maxBBox + minBBox );
			}
			// put center in world-space
			D3DXVec3TransformCoord( &centerBBox, &centerBBox, &m_worldTransform );
			// calc distance to camera
			Vector3 d = Tr2Renderer::GetViewPosition() - centerBBox;
			// put together a sortable item
			Tr2MeshAreaItem item;
			item.m_meshArea = meshArea;
			item.m_distance = D3DXVec3LengthSq( &d );
			meshAreasToSort.push_back( item );
		}
	}

	// Sort the list back to front
	std::sort( meshAreasToSort.begin(), meshAreasToSort.end() );


	for( Tr2MeshAreaItemList::iterator it = meshAreasToSort.begin(); it != meshAreasToSort.end(); ++it )
	{
		Tr2MeshArea* area = it->m_meshArea;
		ITr2ShaderMaterial* material = area->GetMaterialInterface();
		if( area->IsHidden() || !material )
		{
			continue;
		}
		TriGeometryBatch* batch = batches->Allocate<TriGeometryBatch>();
		// Note that this can fail if the accumulator can't add more batches!
		if( batch )
		{
			batch->SetShaderMaterial( material );
			batch->SetPerObjectData( perObjectData );
			batch->SetGeometryResource( geomRes );
			batch->SetMeshParameters( meshIx, area->GetIndex(), area->GetCount() );

			batches->Commit( batch );
		}
	}
}

// ---------------------------------------------------------------------------------------
//  Description:
//    Given a pointer to a mesh overlay vector, gathers TriGeometryBatches for each.
// ---------------------------------------------------------------------------------------
void EveSpaceObject2::GetBatchesFromOverlayVector( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData, TriBatchType batchType )
{
	if( m_overlayEffects.empty() )
	{
		return;
	}

	TriGeometryRes* geomRes = NULL;
	if( !geomRes )
	{
		geomRes = m_mesh->GetGeometryResource();
	}

	if( !geomRes || !geomRes->IsGood() )
	{
		return;
	}

	int meshIx = m_mesh->GetMeshIndex();

	for( auto it = m_overlayEffects.begin(); it != m_overlayEffects.end(); ++it )
	{
		EveMeshOverlayEffectPtr overlay = *it;
		bool success = false;
		const PTr2EffectVector& effects = overlay->GetEffects( batchType, success );
		if ( success )
		{
			EveMeshOverlayEffect::OverlayType overlayType = overlay->GetType( batchType );
			for( auto eff = effects.begin(); eff != effects.end(); ++eff )
			{
				Tr2EffectPtr effect = *eff;

				// add all mesh area blocks
				for( auto areaBlock = m_overlayMeshAreaBlocks[overlayType].begin(); areaBlock != m_overlayMeshAreaBlocks[overlayType].end(); ++areaBlock )
				{
					TriGeometryBatch* batch = batches->Allocate<TriGeometryBatch>();
					// Note that this can fail if the accumulator can't add more batches!
					if( batch )
					{
						batch->SetShaderMaterial( effect );
						batch->SetPerObjectData( perObjectData );
						batch->SetGeometryResource( geomRes );
						batch->SetMeshParameters( meshIx, areaBlock->m_startIndex, areaBlock->m_count );
						batches->Commit( batch );
					}
				}
			}
		}
	}
}

void EveSpaceObject2::RebuildDamageLocatorCache()
{
	if( m_transformedDamageLocators )
	{
		CCP_ALIGNED_FREE( m_transformedDamageLocators );
	}

	if( m_damageLocatorPositions )
	{
		CCP_ALIGNED_FREE( m_damageLocatorPositions );
	}

	// We still need to ensure the data lives in aligned memory for runtime use
	m_allocatedDamageLocatorCount = (unsigned int)m_persistedDamageLocators.size();
	m_persistedImpactDirectionCount = (unsigned int)m_persistedDamageLocators.size();
	
	unsigned int memSize = m_allocatedDamageLocatorCount*sizeof(XMVECTOR);
	m_damageLocatorPositions = (XMVECTOR*)CCP_ALIGNED_MALLOC( "EveSpaceObject2/m_damageLocatorPositions", memSize, 16 );
	m_transformedDamageLocators = (XMVECTOR*)CCP_ALIGNED_MALLOC( "EveSpaceObject2/m_transformedDamageLocators", memSize, 16 );
	m_transformedImpactDirections = (XMVECTOR*)CCP_ALIGNED_MALLOC( "EveSpaceObject2/m_transformedImpactDirections", memSize, 16 );


	for ( unsigned int i = 0; i <  m_allocatedDamageLocatorCount; ++i )
	{
		// Use the implicit conversion of Vector3 to XMVECTOR
		m_damageLocatorPositions[i] = m_persistedDamageLocators[i].m_position;
	}
}

const Matrix* EveSpaceObject2::GetLocatorTransform( LocatorType lt, unsigned int lix )
{
	switch( lt )
	{
	case ELT_TRANSFORM:
		{
			EveLocator2* t = m_locators[lix];
			return &t->GetTransform();
		}
		break;

	case ELT_JOINT:
		{
			if( !m_animationUpdater || !m_animationUpdater->m_worldPose )
			{
				return NULL;
			}

			return (const Matrix*)GrannyGetWorldPose4x4( m_animationUpdater->m_worldPose, lix );
		}
		break;

	default:
		return nullptr;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Scan through all the attached locators and compare their names with search
//   name. This can tell us how many single turrets one turret set has.
// Arguments:
//   namePrefix - substring to match locator's name
// Return Value:
//   The number of locators that have namePrefix in their name
// SeeAlso:
//   EveTurretSet, EveLocatorType, EveLocator2
// --------------------------------------------------------------------------------
unsigned int EveSpaceObject2::CountLocatorsByPrefix( const char* namePrefix ) const
{
	// want all?
	if( namePrefix == NULL)
	{
		return (unsigned int)m_locators.size();
	}

	// now, so count them
	unsigned int count = 0;
	for( PEveLocator2Vector::const_iterator it = m_locators.begin(); it != m_locators.end(); ++it )
	{
		if( strncmp( (*it)->GetName(), namePrefix, strlen( namePrefix ) ) == 0 )
		{
			++count;
		}
	}
	return count;
}

// --------------------------------------------------------------------------------
// Description:
//   Determines whether a locator is a bone joint or locator
// --------------------------------------------------------------------------------
EveSpaceObject2::LocatorType EveSpaceObject2::DetermineLocatorType( const char* locatorName, unsigned int& locatorIndex ) const
{
	EveSpaceObject2::LocatorType ret = ELT_COUNT;
	// using a bone's position has priority!
	if( FindLocatorJointByName( locatorName, locatorIndex ) )
	{
		ret = ELT_JOINT;
	}
	else if( FindLocatorTransformByName( locatorName, locatorIndex ) )
	{
		ret = ELT_TRANSFORM;
	}
	return ret;
}

bool EveSpaceObject2::FindLocatorJointByName( const char* name, unsigned int& ix ) const
{
	return m_animationUpdater ? m_animationUpdater->FindBoneByName( name, ix ) : false;
}

bool EveSpaceObject2::FindLocatorTransformByName( const char* name, unsigned int& ix ) const
{
	unsigned int n = (unsigned int)m_locators.size();
	for( unsigned int i = 0; i < n ; ++i )
	{
		const char* locatorName = m_locators[i]->GetName();
		if( strcmp( name, locatorName ) == 0 )
		{
			ix = i;
			return true;
		}
	}

	return false;
}

void EveSpaceObject2::UpdateShLighting( Tr2ShLightingManager& manager )
{
	memset( m_shLightingCoefficients, 0, sizeof( m_shLightingCoefficients ) );
	if( m_estimatedPixelDiameterWithChildren > g_eveSpaceSceneLowDetailThreshold )
	{
		float intensityFadeRadius = ( g_eveSpaceSceneMediumDetailThreshold - g_eveSpaceSceneLowDetailThreshold ) * 0.25f;
		float intensity = ( m_estimatedPixelDiameterWithChildren - g_eveSpaceSceneLowDetailThreshold ) / intensityFadeRadius;
		intensity = std::min( std::max( intensity, 0.f ), 1.f );
		manager.GetLighting( m_worldPosition, intensity, m_boundingSphereRadius * g_secondaryLightingRadiusCutoffFactor, m_shLightingCoefficients );
	}
}

void EveSpaceObject2::ClearShLighting()
{
	memset( m_shLightingCoefficients, 0, sizeof( m_shLightingCoefficients ) );
}

// --------------------------------------------------------------------------------
// Description:
//   Create the per-object data block and fill it with information only the
//   spaceobject knows.
// SeeAlso:
//   EveSpaceObjectPerObjectData, TriRenderBatchAccumulator
// --------------------------------------------------------------------------------
Tr2PerObjectData* EveSpaceObject2::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	EveSpaceObjectPerObjectData* perObjectData = accumulator->Allocate<EveSpaceObjectPerObjectData>();
	if( !perObjectData )
	{
		return NULL;
	}
	perObjectData->Initialize( this, &m_perObjectDataVs, &m_perObjectDataPs );
	perObjectData->m_psPointLightCount = m_psPointLightCount;

	return perObjectData;
}

uint32_t EveSpaceObject2::GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const
{
	if( shaderType == Tr2RenderContextEnum::PIXEL_SHADER )
	{
		uint32_t sz = 16 + 16 + 16 + sizeof( m_shLightingCoefficients ); // m_spaceObjectMiscData + m_spaceObjectClipData + m_spaceObjectClipDataEx
		if( m_customMask )
		{
			sz += 64 + 16;  // customMask data is optional for now
		}
		return sz;
	}
	else
	{
		int boneCount = 0;
		if( m_animationUpdater && m_animationUpdater->IsInitialized() )
		{
			boneCount = m_animationUpdater->GetMeshBoneCount();
		}

		return
			64 +				// m_vsWorldMatrix
			16 +				// m_vsSpaceObjectData
			16 +				// m_spaceObjectClipData
			boneCount * 3 * 16;	// m_vsBonesMatrix (3x4)
	}
}

void EveSpaceObject2::UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* data )
{
	m_spaceObjectMiscData.w = GetBoundingSphereRadius();

	if( shaderType == Tr2RenderContextEnum::PIXEL_SHADER )
	{
		uint8_t* perObjectPS = (uint8_t*)data;

		memcpy( perObjectPS, &m_spaceObjectMiscData, sizeof( Vector4 ) );
		perObjectPS += sizeof( Vector4 );
		memcpy( perObjectPS, &m_spaceObjectClipData, sizeof( Vector4 ) );
		perObjectPS += sizeof( Vector4 );
		memcpy( perObjectPS, &m_spaceObjectClipDataEx, sizeof( Vector4 ) );
		perObjectPS += sizeof( Vector4 );
		memcpy( perObjectPS, m_shLightingCoefficients, sizeof( m_shLightingCoefficients ) );
		perObjectPS += sizeof( m_shLightingCoefficients );

		if( m_customMask )
		{
			m_customMask->GetInvCustomMaskTransform( (Matrix*)perObjectPS );
			perObjectPS += sizeof( Matrix );

			m_customMask->GetExtendedData( (Vector4*)perObjectPS );
			perObjectPS += sizeof( Vector4 );
		}
	}
	else
	{
		uint8_t* perObjectVS = (uint8_t*)data;

		D3DXMatrixTranspose( reinterpret_cast<Matrix*>( perObjectVS ), &m_worldTransform );
		perObjectVS += sizeof( Matrix );
		memcpy( perObjectVS, &m_spaceObjectMiscData, sizeof( Vector4 ) );
		perObjectVS += sizeof( Vector4 );
		memcpy( perObjectVS, &m_spaceObjectClipData, sizeof( Vector4 ) );
		perObjectVS += sizeof( Vector4 );

		// maybe animated?
		size -= 64 + 16 + 16;
		if( size )
		{
			memcpy( perObjectVS, m_animationUpdater->GetMeshBoneMatrixList(), size );
		}
	}
}

Vector4 EveSpaceObject2::CalculateSkinnedBoundingSphere()
{
	if( m_isAnimated )
	{
		return m_animationUpdater->CalculateSkinnedBoundingSphere( m_mesh->GetGeometryResource()->GetGrannyInfo() );
	}
	return Vector4( 0, 0, 0, -1 );
}

std::pair<Vector3, Vector3> EveSpaceObject2::CalculateSkinnedBoundingBoxFromTransform( const Matrix& transform )
{
	Vector3 bbMin, bbMax;
	BoundingBoxInitialize( bbMin, bbMax );
	if( m_isAnimated )
	{
		m_animationUpdater->CalculateSkinnedBoundingBoxFromTransform( transform, bbMin, bbMax, m_geometryResFromMesh->GetGrannyInfo() );
	}
	return std::pair<Vector3, Vector3>( bbMin, bbMax );
}

void EveSpaceObject2::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_isVisible = false;
	m_isMeshVisible = false;

	if( !m_display )
	{
		return;
	}
	m_lodLevel = TR2_LOD_LOW;
	m_lodLevelWithChildren = TR2_LOD_LOW;

	if( m_boundingSphereRadius > 0.0f )
	{
		if( frustum.IsSphereVisible( &m_boundingSphereWorld ) )
		{
			m_estimatedPixelDiameter = frustum.GetPixelSizeAccross( &m_boundingSphereWorld );
			m_isMeshVisible = true;
		}
	}

	Vector4 bounds;
	if( GetBoundingSphere( bounds, EVE_BOUNDS_WITH_CHILDREN ) )
	{
		m_isInFrustum = frustum.IsSphereVisible( &bounds );
		m_estimatedPixelDiameterWithChildren = frustum.GetPixelSizeAccross( &bounds );
		if( m_estimatedPixelDiameterWithChildren >= g_eveSpaceSceneVisibilityThreshold )
		{
			m_isVisible = true;
		}
	}

	if( m_isVisible )
	{
		if( g_lodLevelUltraEnabled && m_estimatedPixelDiameter > g_eveSpaceSceneHighDetailThreshold )
		{
			m_lodLevel = TR2_LOD_ULTRA;
		}
		else if( m_estimatedPixelDiameter > g_eveSpaceSceneMediumDetailThreshold )
		{
			m_lodLevel = TR2_LOD_HIGH;
		}
		else if( m_estimatedPixelDiameter > g_eveSpaceSceneLowDetailThreshold )
		{
			m_lodLevel = TR2_LOD_MEDIUM;
		}
		else
		{
			m_lodLevel = TR2_LOD_LOW;
		}

		if( g_lodLevelUltraEnabled && m_estimatedPixelDiameterWithChildren > g_eveSpaceSceneHighDetailThreshold )
		{
			m_lodLevelWithChildren = TR2_LOD_ULTRA;
		}
		else if( m_estimatedPixelDiameterWithChildren > g_eveSpaceSceneMediumDetailThreshold )
		{
			m_lodLevelWithChildren = TR2_LOD_HIGH;
		}
		else if( m_estimatedPixelDiameterWithChildren > g_eveSpaceSceneLowDetailThreshold )
		{
			m_lodLevelWithChildren = TR2_LOD_MEDIUM;
		}
		else
		{
			m_lodLevelWithChildren = TR2_LOD_LOW;
		}

		if( m_allowLodSelection && m_isMeshVisible )
		{
			SelectMeshLevelOfDetail();
		}
		if( m_mesh && !m_mesh->IsLoading() && m_isMeshVisible )
		{
			renderables.push_back( this );

			if( m_estimatedPixelDiameter >= g_eveSpaceSceneLowDetailThreshold )
			{
				CCP_STATS_INC( eveHighDetailObjects );
			}
			else
			{
				CCP_STATS_INC( eveLowDetailObjects );
			}
		}

		if( DisplayChildren() )
		{
			for( IEveTransformVector::const_iterator it = m_children.begin(); it != m_children.end(); ++it )
			{
				IEveTransform* p = *it;
				p->GetRenderables( frustum, renderables, m_worldTransform );
			}
		}

		// are decals visible?
		if( DisplayDecals() && m_mesh && m_isMeshVisible )
		{
			TriGeometryResPtr geometryRes = m_mesh->GetGeometryResource();
			if( geometryRes )
			{
				for( EveSpaceObjectDecalVector::const_iterator it = m_decals.begin(); it != m_decals.end(); ++it )
				{
					// assign this space-object's cache to the decal
					(*it)->SetCache( m_decalCache );
					// tell the decal of animation, IF we have any
					if( m_animationUpdater && m_animationUpdater->GetMeshBoneCount() && m_animationUpdater->IsInitialized() )
					{
						(*it)->SetBoneMatrix( m_animationUpdater->GetMeshBoneMatrixList(), m_animationUpdater->GetMeshBoneCount() );
					}
					// now prep to get the renderables
					EveSpaceObjectDecal::ParentData pd;
					pd.transform = m_worldTransform;
					pd.shipData = m_spaceObjectMiscData;
					pd.clipData = m_spaceObjectClipData;
					pd.clipDataEx = m_spaceObjectClipDataEx;
					pd.shLighting = m_shLightingCoefficients;
					(*it)->GetRenderables( geometryRes, frustum, renderables, &pd );
				}
				m_decalCache->Clear();
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Registers space object attachments (sprite and spotlight sets) with quad 
//   renderer.
// Arguments:
//   quadRenderer - quad renderer
// --------------------------------------------------------------------------------
void EveSpaceObject2::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	for( auto it = m_spriteSets.begin(); it != m_spriteSets.end(); ++it )
	{
		(*it)->RegisterWithQuadRenderer( quadRenderer );
	}
	for( auto it = m_spotlightSets.begin(); it != m_spotlightSets.end(); ++it )
	{
		(*it)->RegisterWithQuadRenderer( quadRenderer );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Adds sprites from sprite sets and spotlight sets to quad renderer.
// Arguments:
//   quadRenderer - quad renderer
// --------------------------------------------------------------------------------
void EveSpaceObject2::AddQuadsToQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	if( !m_isInFrustum )
	{
		return;
	}
	size_t boneCount = 0;
	const granny_matrix_3x4* bones = nullptr;
	if( m_animationUpdater && m_animationUpdater->IsInitialized() )
	{
		boneCount = size_t( m_animationUpdater->GetMeshBoneCount() );
		if( boneCount )
		{
			bones = m_animationUpdater->GetMeshBoneMatrixList();
		}
	}

	for( auto it = m_spriteSets.begin(); it != m_spriteSets.end(); ++it )
	{
		(*it)->AddToQuadRenderer( quadRenderer, m_worldTransform, m_spaceObjectMiscData.y, bones, boneCount );
	}
	for( auto it = m_spotlightSets.begin(); it != m_spotlightSets.end(); ++it )
	{
		(*it)->AddToQuadRenderer( quadRenderer, m_worldTransform, m_spaceObjectMiscData.y, m_spaceObjectMiscData.x, bones, boneCount );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Gets shadow caster renderables.
//   Warning: this function can be called on different threads, so it needs to be
//   thread-safe.
// --------------------------------------------------------------------------------
bool EveSpaceObject2::GetRenderablesCastingShadow( bool isSelf, const TriFrustumOrtho& frustum, std::vector<ITr2Renderable*>& renderables )
{
	if( !m_display )
	{
		return false;
	}

	if( m_boundingSphereWorld.w > 0.0f  )
	{
		if( frustum.IsSphereVisibleAndInsideNearPlane( &m_boundingSphereWorld ) )
		{
			renderables.push_back( this );
			return true;
		}
	}
	return false;
}

void EveSpaceObject2::SetMesh( Tr2MeshBase* mesh )
{
	m_mesh = mesh;
	m_allowLodSelection = false;
}

float EveSpaceObject2::GetBoundingSphereRadius( )
{
	if( m_dynamicBoundingSphere.w != -1 )
	{
		return m_modelScale * m_dynamicBoundingSphere.w;
	}
	return m_modelScale * m_boundingSphereRadius;
}

Vector3 EveSpaceObject2::GetBoundingSphereCenter( )
{
	if( m_dynamicBoundingSphere.w != -1 )
	{
		return Vector3( m_dynamicBoundingSphere.x, m_dynamicBoundingSphere.y, m_dynamicBoundingSphere.z );
	}
	return m_boundingSphereCenter;
}

// --------------------------------------------------------------------------------
// Description:
//   This function returns the number of bones in the granny file. Mainly used
//   for pipeline tools to check we don't run too high on this.
// Return value:
//   Returns the number of bones in the gr2 file
// --------------------------------------------------------------------------------
int EveSpaceObject2::GetBoneCount() const
{
	if( !m_animationUpdater->m_meshBinding )
	{
		return 0;
	}
	return GrannyGetMeshBindingBoneCount( m_animationUpdater->m_meshBinding );
}

bool EveSpaceObject2::RebuildBoundingSphereInformation()
{
	Tr2MeshBase* mesh = m_mesh;

	if( !mesh )
	{
		return false;
	}

	TriGeometryRes* geomRes = mesh->GetGeometryResource();
	if( !geomRes || !geomRes->IsGood() )
	{
		return false;
	}

	geomRes->RecalculateBoundingSphere();
	Vector4 sphere;
	geomRes->GetBoundingSphere( mesh->GetMeshIndex(), sphere );
	m_boundingSphereCenter = Vector3( sphere.x, sphere.y, sphere.z );
	m_boundingSphereRadius = sphere.w;

	return true;
}

void EveSpaceObject2::ReleaseCachedData( BlueAsyncRes* p )
{
	CCP_ASSERT( p == m_geometryResFromMesh );

	FreeAnimationData();

	// no more overlay effects
	for( int i = 0; i < EveMeshOverlayEffect::TYPE_COUNT; ++i )
	{
		m_overlayMeshAreaBlocks[i].clear();
	}
}

void EveSpaceObject2::RebuildCachedData( BlueAsyncRes* p )
{
	// If we already have a model we don't want to go through here
	// as it would nuke all current animations.
	if( !m_animationUpdater || m_animationUpdater->IsInitialized() )
	{
		return;
	}

	if( !m_geometryResFromMesh || !m_geometryResFromMesh->IsGood() )
	{
		m_geometryResFromMesh = nullptr;
		return;
	}
	// Objects with only one animation just have a default animation and aren't treated as
	// animated in terms of bounds calculations and such.
	m_isAnimated = m_geometryResFromMesh && m_geometryResFromMesh->GetAnimationCount() > 1;

	m_animationUpdater->SetUseMeshBinding( true );	
	m_animationUpdater->SetSharedGeometryRes( m_geometryResFromMesh );
	m_animationUpdater->RebuildCachedData( p );

	if( m_animationUpdater->IsInitialized() )
	{
		Matrix m;
		D3DXMatrixIdentity( &m );
		m_animationUpdater->PrePhysicsAnimation( 0, m );
	}

	if( m_animationUpdater->IsInitialized() && m_animationSequencer )
	{
		m_animationSequencer->SetOwner( this );
	}
	else if( m_geometryResFromMesh->GetAnimationCount() == 0 && m_animationSequencer )
	{
		m_animationSequencer->SetOwner( this );
	}

	if( m_boundingSphereRadius < 0.0f )
	{
		CCP_LOGWARN( "Bounding sphere not set for '%s' - calculating from '%s'", m_name.c_str(), m_geometryResFromMesh->GetPath() );
		m_geometryResFromMesh->RecalculateBoundingSphere();
		Vector4 sphere;
		m_geometryResFromMesh->GetBoundingSphere( m_mesh->GetMeshIndex(), sphere );
		m_boundingSphereCenter = Vector3( sphere.x, sphere.y, sphere.z );
		m_boundingSphereRadius = sphere.w;
	}

	// build list of block areas we need to render for overlay effects
	if( m_mesh )
	{
		m_mesh->CollectAreaBlocks( m_overlayMeshAreaBlocks[EveMeshOverlayEffect::TYPE_ALL], TRIBATCHTYPE_OPAQUE );
		m_mesh->CollectAreaBlocks( m_overlayMeshAreaBlocks[EveMeshOverlayEffect::TYPE_ALL], TRIBATCHTYPE_TRANSPARENT );
		m_mesh->CollectAreaBlocks( m_overlayMeshAreaBlocks[EveMeshOverlayEffect::TYPE_ALL], TRIBATCHTYPE_DECAL );
		m_mesh->CollectAreaBlocks( m_overlayMeshAreaBlocks[EveMeshOverlayEffect::TYPE_OPAQUEONLY], TRIBATCHTYPE_OPAQUE );
		// this list is too long will hold one element for each mesharea at least... Optimize!
		for( int i = 0; i < EveMeshOverlayEffect::TYPE_COUNT; ++i )
		{
			TriRenderBatchAreaBlock::Optimize( m_overlayMeshAreaBlocks[i] );
		}
	}
}

bool EveSpaceObject2::OnModified( Be::Var* val )
{
	if( IsMatch( val, m_animationSequencer ) && m_animationSequencer && m_animationUpdater->IsInitialized() )
	{
		m_animationSequencer->SetOwner( this );
	}
	return true;
}

bool EveSpaceObject2::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	if( m_boundingSphereRadius <= 0.0f && m_dynamicBoundingSphere.w <= 0 )
	{
		return false;
	}

	sphere = m_boundingSphereWorld;
	if( query == EVE_BOUNDS_NORMAL || !DisplayChildren() )
	{
		return true;
	}

	Vector4 childBounds;
	for( auto it = m_children.begin(); it != m_children.end(); it++ )
	{
		if( (*it)->GetBoundingSphere( childBounds, query ) )
		{
			BoundingSphereUpdate( childBounds, sphere );
		}
	}
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Update view distance info using this object's bounds. Should be called AFTER
//   GetRenderables is called or the object might be ignored.
// --------------------------------------------------------------------------------
void EveSpaceObject2::UpdateViewDistanceInfo( const TriFrustum& frustum, ViewDistanceInfo& viewDistance ) const
{
	if( !m_display || !m_isVisible )
	{
		return;
	}

	Vector4 v;
	if( GetBoundingSphere( v ) )
	{
		viewDistance.UpdateClipPlanes( v, frustum );
	}
	for( auto it = m_children.begin(); it != m_children.end(); it++ )
	{
		(*it)->UpdateViewDistanceInfo( frustum, viewDistance );
	}
	for( auto it = m_spotlightSets.begin(); it != m_spotlightSets.end(); it++ )
	{
		(*it)->UpdateViewDistanceInfo( frustum, viewDistance, m_worldTransform );
	}
	for( auto it = m_planeSets.begin(); it != m_planeSets.end(); it++ )
	{
		(*it)->UpdateViewDistanceInfo( frustum, viewDistance, m_worldTransform );
	}
}

void EveSpaceObject2::PlayAnimation( const char* animName, bool replace, int loopCount, float delay, float speed )
{
	if( m_animationUpdater )
	{
		m_animationUpdater->PlayAnimation( animName, replace, loopCount, delay, speed );		
	}
}

void EveSpaceObject2::PlayAnimationOnce( const char* animName )
{
	PlayAnimation( animName, true, 1, 0.0f, 1.0f );
}

void EveSpaceObject2::PlayAnimationEx( const char* animName, int loopCount, float start, float speed )
{
	PlayAnimation( animName, true, loopCount, start, speed );
}

void EveSpaceObject2::ChainAnimation( const char* animName )
{
	PlayAnimation( animName, false, 1, 0.0f, 1.0f );
}

void EveSpaceObject2::ChainAnimationEx( const char* animName, int loopCount, float start, float speed )
{
	PlayAnimation( animName, false, loopCount, start, speed );
}

void EveSpaceObject2::EndAnimation()
{
	if( m_animationUpdater )
	{
		m_animationUpdater->EndAnimation();		
	}
}

void EveSpaceObject2::ClearAnimations()
{
	if( m_animationUpdater )
	{
		m_animationUpdater->ClearAnimations();		
	}
}

int EveSpaceObject2::GetClosestDamageLocatorIndex( const Vector3* position )
{
	// Run a single pass on the transformed damage locator positions and select the closest
	float max = std::numeric_limits<float>::max();
	XMVECTOR closestLength = XMVectorSet( max, max, max, 0.0 );
	int closestIndex = -1;

	{
		CcpAutoMutex lock( GetObjectMutex() );
		if( !m_damageLocatorsUpdatedThisFrame )
		{
			UpdateDamageLocatorPositions();
		}
	}

	for( unsigned int i = 0; i < m_allocatedDamageLocatorCount ; ++i )
	{
		XMVECTOR thisLength = *position;
		thisLength = m_transformedDamageLocators[i] - thisLength;
		thisLength = XMVector3LengthEst( thisLength );
		if ( XMVector3Less( thisLength, closestLength ) )
		{
			closestIndex = i;
			closestLength = thisLength;
		}
	}

	return closestIndex;
}

int EveSpaceObject2::GetInterestingDamageLocatorIndex( const Vector3 &position ) const
{
	const static XMVECTOR reference = XMVectorSet( 0.7f, 0.7f, 0.7f, 0.7f );
	XMVECTOR offset, centre;
	if( m_ballPosition ) {
		centre = XMVectorSet( m_worldTransform._41, m_worldTransform._42, m_worldTransform._43, 0 );
		offset = XMVector3Normalize( position - centre );
	} else {
		centre = XMVectorSet(0,0,0,0);
		offset = XMVector3Normalize( position );
	}

	std::vector<unsigned> validIndices;
	for( unsigned i = 0; i < m_allocatedDamageLocatorCount; ++i ) {
		auto local = XMVector3Normalize( m_damageLocatorPositions[i] - centre );
		if( XMVector3Greater( XMVector3Dot( offset, local ), reference ) ) {
			validIndices.push_back(i);
		}
	}

	if( validIndices.size() )
		return validIndices[ rand() % validIndices.size() ];
	return -1;
}

static float GetDistanceFit( float minDist, float fitScale, Vector3& vec )
{
	float length = D3DXVec3Length( &vec );
	float scale = 1 - ( length - minDist ) / fitScale;
	float val = 2 * scale - 1.0f;
	if( val < 0 )
	{
		val = 1 - pow( std::abs( val ), 0.5f );
	}
	else
	{
		val = pow( std::abs( val ), 0.5f ) + 1;
	}
	val = val * 0.5f;
	return val;
}

static float GetDirectionFit( const Vector3& v0, const Vector3& v1 )
{
	float d = -D3DXVec3Dot( &v0, &v1 );
	if( d < 0 )
	{
		return ( 1 - pow( std::abs( d ), 0.5f ) ) * 0.5f;
	}
	return ( pow( std::abs( d ), 0.5f ) + 1 ) * 0.5f;
}

int EveSpaceObject2::GetGoodDamageLocatorIndex( const Vector3& position )
{
	bool validImpactDirections = m_persistedImpactDirectionCount == m_allocatedDamageLocatorCount;

	float minDistance = FLT_MAX;
	float maxDistance = FLT_MIN;
	float bestDirectionFit = 0.0f;
	if( !validImpactDirections )
	{
		bestDirectionFit = 1.0f;
	}

	{
		CcpAutoMutex lock( GetObjectMutex() );
		if( !m_damageLocatorsUpdatedThisFrame )
		{
			UpdateDamageLocatorPositions();
		}
		if( !m_impactDirectionsUpdatedThisFrame )
		{
			UpdateImpactDirections();
		}
	}

	Vector3 v;
	for( unsigned i = 0; i < m_allocatedDamageLocatorCount; ++i )
	{
		v = XMVectorSubtract( m_transformedDamageLocators[i], position );
		float length = D3DXVec3Length( &v );
		minDistance = min( minDistance, length );
		maxDistance = max( maxDistance, length );
		D3DXVec3Normalize( &v, &v );
		if( validImpactDirections )
		{
			float directionFit = GetDirectionFit( (Vector3)m_transformedImpactDirections[i], v );
			bestDirectionFit = max( bestDirectionFit, directionFit );
		}
	}

	float desiredFit = TriRand() * ( 0.25f - ( 1.0f - bestDirectionFit ) ) + 0.75f;
	float bestFit = 1.0f;
	int bestLocator = GetClosestDamageLocatorIndex( &position );
	for( unsigned i = 0; i < m_allocatedDamageLocatorCount; ++i )
	{
		v = XMVectorSubtract( m_transformedDamageLocators[i], position );
		float fitValue = GetDistanceFit( minDistance, maxDistance - minDistance, v );
		D3DXVec3Normalize( &v, &v );
		if( validImpactDirections )
		{
			fitValue *= GetDirectionFit( (Vector3)m_transformedImpactDirections[i], v );
		}
		else
		{
			Vector3 tmp( m_transformedImpactDirections[i] );
			fitValue *= D3DXVec3Dot( &tmp, &v ) < 0.f ? 1.0f : 0.1f;
		}
		if( std::abs( fitValue - desiredFit ) < bestFit )
		{
			bestFit = std::abs( fitValue - desiredFit );
			bestLocator = (int)i;
		}
	}

	return bestLocator;
}

bool EveSpaceObject2::GetDamageLocatorPosition( Vector3* out, int index )
{
	if( (index < 0) || ((unsigned int)index >= m_allocatedDamageLocatorCount) || !m_damageLocatorPositions || !m_transformedDamageLocators )
	{
		out->x = m_worldTransform._41;
		out->y = m_worldTransform._42;
		out->z = m_worldTransform._43;
		return false;
	}

	{
		CcpAutoMutex lock( GetObjectMutex() );
		if( !m_damageLocatorsUpdatedThisFrame )
		{
			UpdateDamageLocatorPositions();
		}
	}

	*out = m_transformedDamageLocators[ index ];

	return true;
}

void EveSpaceObject2::GetModelCenterWorldPosition( Vector3 &position, Be::Time t )
{
	// We are being looked at by a camera, so we need to make sure we update early enough
	UpdateWorldTransform( t );

	if( m_isAnimated && m_animationUpdater && m_animationUpdater->IsInitialized() )
	{
		Vector4 boundingSphere;
		m_animationUpdater->GetDynamicBounds( boundingSphere, m_localAabbMin, m_localAabbMax );
		D3DXVec3TransformCoord( &position, (Vector3*)&boundingSphere, &m_worldTransform );
	}
	else
	{
		D3DXVec3TransformCoord( &position, &m_boundingSphereCenter, &m_worldTransform );
	}
}

Vector3 EveSpaceObject2::GetModelWorldPosition()
{
	return Vector3( m_boundingSphereWorld.x, m_boundingSphereWorld.y, m_boundingSphereWorld.z );
}

void EveSpaceObject2::GetMissPosition( const Vector3* hit, const Vector3* source, Vector3* out )
{
	if( m_boundingSphereRadius > 0.0f )
	{
		out->x = m_boundingSphereWorld.x;
		out->y = m_boundingSphereWorld.y;
		out->z = m_boundingSphereWorld.z;
		
		if( hit && source ) 
		{
			Vector3 local( *hit - *out );
			Vector3 dir( *hit - *source );
			
			D3DXVec3Normalize( &dir, &dir );
			local -= dir * D3DXVec3Dot( &dir, &local );

			D3DXVec3Normalize( &local, &local );
			const Vector3 off = local * m_boundingSphereWorld.w * 1.125f;
			*out += off;
		}
	}
	else
	{
		GetDamageLocatorPosition( out, -1 );
	}
}

const Vector3* EveSpaceObject2::GetWorldPosition()
{
	return &m_worldPosition;
}

void EveSpaceObject2::SelectMeshLevelOfDetail()
{
	Tr2MeshBase* mesh = NULL;

	if( m_meshLod )
	{
		m_meshLod->SelectLod( static_cast<Tr2Lod>( m_lodLevel ) );
		m_mesh = m_meshLod;
	}
	else
	{
		// still use the original mesh, which then acts as LOD_HIGH
		m_lodLevel = TR2_LOD_HIGH;
	}

	if( m_mesh )
	{
		PrepareForAnimation();
		m_mesh->GetBoundingBox( m_localAabbMin, m_localAabbMax );
	}
}

void EveSpaceObject2::UnloadLodIfNeeded( Be::Time time )
{
	if( EveSpaceScene::IsMeshUnloadingEnabled() )
	{
		m_mesh = NULL;
	}
}

void EveSpaceObject2::UpdateWorldTransform( Be::Time time )
{
	Quaternion rotation( 0.0f, 0.0f, 0.0f, 1.0f );

	if( m_ballPosition )
	{
		m_ballPosition->Update( &m_worldPosition, time );
	}
	else
	{
		m_worldPosition = Vector3( 0.0f, 0.0f, 0.0f );
	}

	if( m_ballRotation )
	{
		m_ballRotation->Update( &rotation, time );
	}

	if( m_modelRotation )
	{
		Quaternion modelRotation;
		m_modelRotation->Update( &modelRotation, time );
		D3DXQuaternionMultiply( &rotation, &modelRotation, &rotation);
	}

	// This makes the assumption that the EveShip is at the bottom level of scene models list
	D3DXMatrixRotationQuaternion(&m_worldTransform, &rotation);

	// scaling: as of now: ONLY FOR ASTEROIDS!
	if(m_modelScale != 1.f)
	{
		// build and mult scale-matrix
		Matrix scaleMatrix;
		D3DXMatrixScaling(&scaleMatrix, m_modelScale, m_modelScale, m_modelScale);
		D3DXMatrixMultiply(&m_worldTransform, &m_worldTransform, &scaleMatrix);
	}

	if( m_modelTranslation )
	{
		// If there's a translation on the model, then it needs to be applied in the coordinate space
		// of the ship model, but without the ball translation
		Vector3 translation;
		m_modelTranslation->Update( &translation, time );
		D3DXVec3TransformCoord( &translation, &translation, &m_worldTransform);
		m_worldTransform._41 = m_worldPosition.x + translation.x;
		m_worldTransform._42 = m_worldPosition.y + translation.y;
		m_worldTransform._43 = m_worldPosition.z + translation.z;
	}
	else
	{
		m_worldTransform._41 = m_worldPosition.x;
		m_worldTransform._42 = m_worldPosition.y;
		m_worldTransform._43 = m_worldPosition.z;
	}
}

bool EveSpaceObject2::IsShadowReceiveEnabled()
{
	return m_enableShadow && m_shadowEffect;
}

// --------------------------------------------------------------------------------
// Description:
//   Nothing here atm
// --------------------------------------------------------------------------------
void EveSpaceObject2::SetLights( EveSpaceSceneLightMgrPtr lightMgr )
{
	m_lightManager = lightMgr;
	m_psPointLightCount = 0;
	if( m_lightManager )
	{
		for( unsigned int i = 0; i < m_lightManager->GetStaticPointlightCount(); ++i )
		{
			const EveSpaceScenePointLightPtr pointLight = m_lightManager->GetStaticPointlight( i );
			if( pointLight && pointLight->IsDisplay() )
			{
				++m_psPointLightCount;
			}
		}
	}
}

void EveSpaceObject2::GetCurrentModelCenterWorldPosition( Vector3 &position )
{
	// This version of the function does not perform an update on the object
	D3DXVec3TransformCoord( &position, &m_boundingSphereCenter, &m_worldTransform );
}

bool EveSpaceObject2::GetLocalBoundingBox( Vector3 &min, Vector3 &max )
{
	if( m_isAnimated && m_animationUpdater && m_animationUpdater->IsInitialized() )
	{
		Vector4 bs;
		m_animationUpdater->GetDynamicBounds( bs, min, max );
		m_localAabbMin = min;
		m_localAabbMax = max;
		return true;
	}
	else if( m_mesh )
	{
		if( m_mesh->GetBoundingBox( min, max ) )
		{
			m_localAabbMin = min;
			m_localAabbMax = max;
			return true;
		}
	}
	// this may get called before SelectMeshLevelOfDetail was called, in which case we have a null m_mesh.
	// To fix that, store the AABB and return it here; at worst, it lags one frame.
	min = m_localAabbMin;
	max = m_localAabbMax;
	return true;
}
	
void EveSpaceObject2::GetLocalToWorldTransform( Matrix &transform )
{
	transform = m_worldTransform;
}

void EveSpaceObject2::FreezeHighDetailMesh()
{
	if( m_meshLod )
	{
		m_mesh = m_meshLod;
		m_meshLod->SelectLod( TR2_LOD_HIGH );

		m_allowLodSelection = false;
		m_lodLevel = TR2_LOD_HIGH;

		PrepareForAnimation();
	}
}

void EveSpaceObject2::FreeAnimationData()
{
	if( m_animationUpdater )
	{
		m_animationUpdater->Cleanup();		
	}
}

void EveSpaceObject2::PrepareForAnimation()
{
	if( !m_geometryResFromMesh )
	{
		// If this is the first time we see a mesh we set up a callback on the geometry resource
		// file load to check for possible animations. If the file has animations we set up
		// the data structures for animation playback.
		m_geometryResFromMesh = m_mesh->GetGeometryResource();
		if( m_geometryResFromMesh )
		{
			// We might be loading, still. The AddNotifyTarget below will trigger a callback
			// once the loading is done. If the geometry resource has already loaded we get the callback
			// immediately. Further initialization that relies on the granny file being in
			// memory happens in the callback (RebuildCachedData)

			m_geometryResFromMesh->AddNotifyTarget( this );			
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Determines if we render this object's decals or not. Usually we only
//   render them when highest LODs are shown.
// SeeAlso:
//   EveSpaceObjectDecal
// --------------------------------------------------------------------------------
bool EveSpaceObject2::DisplayDecals() const
{
	// if LOD selection is deactivated, it's always highest so decals are on
	if( !m_allowLodSelection )
		return true;
	return m_lodLevel >= TR2_LOD_HIGH;
}

// --------------------------------------------------------------------------------
// Description:
//   Determines if we render this object's children or not.
// SeeAlso:
//   EveTransform
// --------------------------------------------------------------------------------
bool EveSpaceObject2::DisplayChildren() const
{
	return true;
}

void EveSpaceObject2::UpdateDamageLocatorPositions()
{
	if( m_damageLocatorPositions && m_transformedDamageLocators )
	{
		// Update the aligned transform matrix
		*m_alignedTransformMatrix = (XMMATRIX)m_worldTransform;

		// Transform the coordinates
		for( unsigned i = 0; i < m_allocatedDamageLocatorCount; ++i )
		{
			m_transformedDamageLocators[i] = XMVector3TransformCoord( m_damageLocatorPositions[i], *m_alignedTransformMatrix );
		}

		m_damageLocatorsUpdatedThisFrame = true;
	}
}

void EveSpaceObject2::UpdateImpactDirections()
{
	if( !m_transformedImpactDirections )
	{
		return;
	}
		
	// Update the aligned transform matrix
	*m_alignedTransformMatrix = (XMMATRIX)m_worldTransform;

	for( unsigned i = 0; i < m_allocatedDamageLocatorCount; ++i )
	{
		Quaternion quat = m_persistedDamageLocators[i].m_impactDirection;
		m_transformedImpactDirections[i] = XMVector3Rotate( Vector3( 0.f, 1.f, 0.f ), quat );
		m_transformedImpactDirections[i] = XMVector3TransformNormal( m_transformedImpactDirections[i], *m_alignedTransformMatrix );
	}

	m_impactDirectionsUpdatedThisFrame = true;
}

ITriVectorFunctionPtr EveSpaceObject2::GetPositionFunction() 
{ 
	return m_ballPosition; 
}

// --------------------------------------------------------------------------------
// Description:
//   Set the boundingsphere inforamtion from the outside directly, instead of
//   calculating it in runtime
// --------------------------------------------------------------------------------
void EveSpaceObject2::SetBoundingSphereInformation( const Vector4* centerAndRadius )
{
	m_boundingSphereCenter = *(Vector3*)centerAndRadius;
	m_boundingSphereRadius = centerAndRadius->w;
}

// --------------------------------------------------------------------------------
// Description:
//   Add a new spriteset to this object from the outside
// --------------------------------------------------------------------------------
void EveSpaceObject2::AddSpriteSet( EveSpriteSetPtr newSpriteSet )
{
	m_spriteSets.Append( newSpriteSet->GetRawRoot() );
}

// --------------------------------------------------------------------------------
// Description:
//   Add a new spotlightset to this object from the outside
// --------------------------------------------------------------------------------
void EveSpaceObject2::AddSpotlightSet( EveSpotlightSetPtr newSpotlightSet )
{
	m_spotlightSets.Append( newSpotlightSet->GetRawRoot() );
}

// --------------------------------------------------------------------------------
// Description:
//   Add a new planeset to this object from the outside
// --------------------------------------------------------------------------------
void EveSpaceObject2::AddPlaneSet( EvePlaneSetPtr newPlaneSet )
{
	m_planeSets.Append( newPlaneSet->GetRawRoot() );
}

// --------------------------------------------------------------------------------
// Description:
//   Add a new decal to this object from the outside
// --------------------------------------------------------------------------------
void EveSpaceObject2::AddDecal( EveSpaceObjectDecalPtr newDecal )
{
	m_decals.Append( newDecal->GetRawRoot() );
}

// --------------------------------------------------------------------------------
// Description:
//   Add a new curveSet to this object from the outside
// --------------------------------------------------------------------------------
void EveSpaceObject2::AddCurveSet( TriCurveSetPtr newCurveSet )
{
	m_curveSets.Append( newCurveSet->GetRawRoot() );
}

// --------------------------------------------------------------------------------
// Description:
//   Add a new locator to this object from the outside
// --------------------------------------------------------------------------------
void EveSpaceObject2::AddLocator( EveLocator2* newLocator )
{
	m_locators.Append( newLocator );
}

// --------------------------------------------------------------------------------
// Description:
//   Add a new locator to this object from the outside
// --------------------------------------------------------------------------------
void EveSpaceObject2::SetDamageLocators( const EveDamageLocator* damageLocators, size_t damageLocatorCount )
{
	// is a structured list, so we can copy this in one big block
	m_persistedDamageLocators.Resize( damageLocatorCount );
	memcpy( &m_persistedDamageLocators[0], damageLocators, damageLocatorCount * sizeof( EveDamageLocator ) );
}

// --------------------------------------------------------------------------------
// Description:
//   Set the shadow shader of this object from the outside
// --------------------------------------------------------------------------------
void EveSpaceObject2::SetShadowEffect( Tr2EffectPtr newShadowEffect )
{
	m_shadowEffect = newShadowEffect;
}

// --------------------------------------------------------------------------------
// Description:
//   Add a transform to the children list
// --------------------------------------------------------------------------------
void EveSpaceObject2::AddToChildrenList( EveTransformPtr transform )
{
	m_children.Append( transform->GetRawRoot() );
}

// --------------------------------------------------------------------------------
// Description:
//   Add a model rotation curve
// --------------------------------------------------------------------------------
void EveSpaceObject2::SetModelRotationCurve( ITriQuaternionFunctionPtr rotationCurve )
{
	m_modelRotation = rotationCurve;
}

// --------------------------------------------------------------------------------
// Description:
//   Add a model translation curve
// --------------------------------------------------------------------------------
void EveSpaceObject2::SetModelTranslationCurve( ITriVectorFunctionPtr translationCurve )
{
	m_modelTranslation = translationCurve;
}

// --------------------------------------------------------------------------------
// Description:
//   Store the DNA string of this guy
// --------------------------------------------------------------------------------
void EveSpaceObject2::SetDnaString( const char* dna )
{
	m_dna = dna;
}

//GPU ship explosion test
unsigned EveSpaceObject2::GetDamageLocatorCount() const 
{
	return m_allocatedDamageLocatorCount;
}

Vector3 EveSpaceObject2::GetDamageLocator( unsigned index ) const 
{
	if( index > m_allocatedDamageLocatorCount )
		return Vector3(0,0,0);
	return Vector3( (float*)&m_damageLocatorPositions[index] );
}

Vector3 EveSpaceObject2::GetTransformedDamageLocator( unsigned index ) const
{
	if( index > m_allocatedDamageLocatorCount )
	{
		return Vector3(0,0,0);
	}
	
	return Vector3( (float*)&m_transformedDamageLocators[index] );
}

Be::Result<std::string> EveSpaceObject2::GetLocalBoundingBoxFromScript( std::pair<Vector3, Vector3>& result )
{
	Vector3 min, max;

	if( !GetLocalBoundingBox( min, max ) )
	{
		return Be::Result<std::string>( "Couldn't get bounding box" );
	}

	result = std::make_pair( min, max );
	return Be::Result<std::string>();
}

// --------------------------------------------------------------------------------
// Description:
//   Update the curve set with the appropriate name
// --------------------------------------------------------------------------------
void EveSpaceObject2::UpdateCurveSet( const std::string& name, Be::Time time )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( (*it)->GetName() == name )
		{
			(*it)->Update( time, time );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Play the curve set with the appropriate name
// --------------------------------------------------------------------------------
void EveSpaceObject2::PlayCurveSet( const std::string& name )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( (*it)->GetName() == name )
		{
			(*it)->Play();
		}
	}
	for( auto childIt = m_children.begin(); childIt != m_children.end(); childIt++ )
	{
		(*childIt)->PlayCurveSet( name );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Stop the curve set with the appropriate name
// --------------------------------------------------------------------------------
void EveSpaceObject2::StopCurveSet( const std::string& name )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( (*it)->GetName() == name )
		{
			return (*it)->Stop();
		}
	}
	for( auto childIt = m_children.begin(); childIt != m_children.end(); childIt++ )
	{
		(*childIt)->StopCurveSet( name );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Get the duration of the curve set with the appropriate name
// --------------------------------------------------------------------------------
float EveSpaceObject2::GetCurveSetDuration( const std::string& name ) const
{
	float maxDuration = 0.f;
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( (*it)->GetName() == name )
		{
			maxDuration = max( maxDuration, (*it)->GetMaxCurveDuration() );
		}
	}
	for( auto childIt = m_children.begin(); childIt != m_children.end(); childIt++ )
	{
		maxDuration = max( maxDuration, (*childIt)->GetCurveSetDuration( name ) );
	}
	return maxDuration;
}

// --------------------------------------------------------------------------------
// Description:
//   Gets called by the state machine of this object to execute some command.
// Return Value:
//   Returns true if this implementation has handled the command.
// --------------------------------------------------------------------------------
bool EveSpaceObject2::ExecuteAnimationStateCommand( EveAnimationCmd cmd, const std::string& data, const std::map<std::string, float>& parameters )
{
	return false;
}

// --------------------------------------------------------------------------------
// Description:
//   Get a mutex for this object that can be used during UpdateAsyncronous call.
// --------------------------------------------------------------------------------
CcpMutex& EveSpaceObject2::GetObjectMutex()
{
	static const size_t MUTEX_COUNT = 256;
	static CcpMutex* mutexes[MUTEX_COUNT] = { nullptr, };
	static char mutexNames[MUTEX_COUNT][64];
	if( !mutexes[0] )
	{
		for( size_t i = 0; i < MUTEX_COUNT; ++i )
		{
			sprintf_s( mutexNames[i], "GetObjectMutex_%" CCP_SIZET_FORMAT, i );
			mutexes[i] = CCP_NEW( "EveSpaceObject2::GetObjectMutex" ) CcpMutex( "EveSpaceObject2", mutexNames[i], 64 );
		}
	}
	return *mutexes[reinterpret_cast<size_t>( this ) & ( MUTEX_COUNT - 1 )];
}

void EveSpaceObject2::SetMeshLod( Tr2MeshLod* mesh )
{
	m_meshLod = mesh;
	m_mesh.Unlock();
}
