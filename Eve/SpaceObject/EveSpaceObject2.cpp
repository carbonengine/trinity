#include "StdAfx.h"

#include "Utilities/BoundingBox.h"
#include "Utilities/BoundingSphere.h"
#include "Utilities/MatrixUtils.h"

#include "include/ITr2DebugRenderer.h"
#include "include/IEveBallpark.h"
#include "Include/TriMath.h"
#include "Resources/TriGeometryRes.h"
#include "TriFrustumOrtho.h"

#include "Eve/EveTransform.h"
#include "EveSpaceObject2.h"
#include "Eve/EveSpaceScene.h"
#include "Utils/EveLocator2.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpotlightSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EvePlaneSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteLineSet.h"
#include "Attachments/EveImpactOverlay.h"
#include "Tr2MeshLod.h"
#include "Tr2GrannyAnimation.h"
#include "Tr2BindingVector3.h"
#include "Eve/EveUpdateContext.h"
#include "Eve/Animation/EveAnimationSequencer.h"
#include "Shader/Tr2Effect.h"
#include "Utils/EveCustomMask.h"
#include "TriSettingsRegistrar.h"
#include "Tr2PointLight.h"
#include "Shader/Utils/Tr2DataTextureManager.h"
#include "Utilities/StringUtils.h"

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
	{ "boneIndex", Be::INT32_1, 28 }, 
	{0} 
};


// --------------------------------------------------------------------------------
// Description:
//   The base class for most of the objects in a space scene
// --------------------------------------------------------------------------------
EveSpaceObject2::EveSpaceObject2( IRoot* lockobj ) :
	PARENTLOCK( m_persistedDamageLocators ),
	PARENTLOCK( m_decals ),
	PARENTLOCK( m_locators ),
	PARENTLOCK( m_observers ),
	PARENTLOCK( m_locatorSets ),
	PARENTLOCK( m_spriteSets ),
	PARENTLOCK( m_spotlightSets ),
	PARENTLOCK( m_planeSets ),
	PARENTLOCK( m_spriteLineSets ),
	PARENTLOCK( m_children ),
	PARENTLOCK( m_curveSets ),
	PARENTLOCK( m_overlayEffects ),
	PARENTLOCK( m_effectChildren ),
	PARENTLOCK( m_lights ),
	PARENTLOCK( m_externalParameters ),
	m_display( true ),
	m_update( true ),
	m_enableShadow( true ),
	m_allowLodSelection( false ),
	m_estimatedPixelDiameter( 0.f ),
	m_estimatedPixelDiameterWithChildren( 0.f ),
	m_boundingSphereCenter( 0.f, 0.f, 0.f ),
	m_boundingSphereRadius( -1.f ),
	m_boundingSphereWorld( 0.f, 0.f, 0.f, -1.f ),
	m_dynamicBoundingSphere( 0.f, 0.f, 0.f, -1.f ),
	m_albedoColor( 0.f, 0.f, 0.f, 1.f ),
	m_secondaryLightingSphereRadius( 0.f ),
	m_modelScale( 1.f ),
	m_worldPosition( 0.f, 0.f, 0.f ),
	m_worldVelocity( 0.f, 0.f, 0.f ),
	m_worldRotation( 0.f, 0.f, 0.f, 1.f ),
	m_lodLevel( TR2_LOD_UNSPECIFIED ),
	m_lodLevelWithChildren( TR2_LOD_UNSPECIFIED ),
	m_debugShowBoundingBox( true ),
	m_debugShowMeshAreaBoundingBox( false ),
	m_debugRenderDebugInfoForChildren( true ),
	m_debugShowDynamicBounds( true ),
	m_debugShowDamageLocators( false ),
	m_isVisible( false ),
	m_isMeshVisible( false ),
	m_localAabbMin( 0.f, 0.f, 0.f ),
	m_localAabbMax( 0.f, 0.f, 0.f ),
	m_shapeEllipsoidCenter( 0.f, 0.f, 0.f ),
	m_shapeEllipsoidRadius( -1.f, -1.f, -1.f ),
	m_lastCurveUpdateTime( 0 ),
	m_previousPosition( UNINITIALIZED_POSITION, UNINITIALIZED_POSITION, UNINITIALIZED_POSITION ),
	m_spaceObjectShipData( 1.f, 1.f, EVE_SPACEOBJECT_DIRT_LEVEL_DEFAULT, 1.f ),
	m_displayChildren( true ),
	m_dirtLevel( EVE_SPACEOBJECT_DIRT_LEVEL_DEFAULT ),
	m_isAnimated( false )
{
	m_worldTransform = XMMatrixIdentity();
	m_invWorldTransform = XMMatrixIdentity();

	m_positionDelta.CreateInstance();

	m_decalCache = CCP_NEW( "EveSpaceObject2::m_decalCache" ) EveSpaceObjectDecalCache;
	
	m_animationUpdater.CreateInstance();
	m_persistedDamageLocators.SetStructureDefinition( EveDamageLocatorStructureDef );

	memset( &m_psData, 0, sizeof( EveSpaceObjectPSData ) );
	memset( &m_vsData, 0, sizeof( EveSpaceObjectVSData ) );
}

EveSpaceObject2::~EveSpaceObject2()
{
	FreeAnimationData();

	if( m_geometryResFromMesh )
	{
		m_geometryResFromMesh->RemoveNotifyTarget( this );
	}

	CCP_DELETE m_decalCache;
}

bool EveSpaceObject2::Initialize()
{
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

Matrix EveSpaceObject2::GetObserverTransform()
{
	return m_worldTransform;
}

void EveSpaceObject2::UpdateSyncronous( EveUpdateContext& updateContext )
{
	Be::Time time = updateContext.GetTime();
	D3DXMatrixTranspose( &m_vsData.worldTransformLast, &m_worldTransform );
	
	UpdateWorldTransform( time );

	if( !m_update )
	{
		return;
	}
		
	if( m_allowLodSelection )
	{
		UnloadLodIfNeeded( time );
	}
	
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
		D3DXVec3TransformNormal( &m_positionDelta->m_value, &positionDelta, &m_invWorldTransform);
	}
	else
	{
		m_positionDelta->m_value = Vector3( 0.f, 0.f, 0.f );
	}
	m_previousPosition = referencePosition;

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
	Matrix observerTransform = GetObserverTransform();
	for( TriObserverLocalVector::iterator it = m_observers.begin(); it != observersEnd; ++it )
	{
		(*it)->Update( observerTransform );
	}

	// trigger syncronous update of attachements here
	for( auto ecIt = m_effectChildren.begin(); ecIt != m_effectChildren.end(); ++ecIt ) 
	{
		(*ecIt)->UpdateSyncronous( updateContext, this, nullptr );
	}
	if( m_impactOverlay )
	{
		m_impactOverlay->UpdateSyncronous( updateContext, this );
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

	PrepareShaderData( updateContext );
	m_psData.shipData = m_spaceObjectShipData;
	D3DXMatrixTranspose( &m_vsData.worldTransform, &m_worldTransform );
	m_vsData.shipData = m_spaceObjectShipData;

	Vector3 shapeCenter( 0.f, 0.f, 0.f ), shapeRadius( 0.f, 0.f, 0.f );
	GetShapeEllipsoid( shapeCenter, shapeRadius );
	m_vsData.ellpsoidRadii = Vector4( shapeRadius, 0.f );
	m_vsData.ellpsoidCenter = Vector4( shapeCenter, 0.f );

	if( m_impactOverlay )
	{
		m_psData.miscData.y = (float)m_impactOverlay->GetDataTextureOffset();
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

	// trigger syncronous update of attachements here
	for( IEveTransformVector::const_iterator it = m_children.begin(); it != m_children.end(); ++it )
	{
		(*it)->Update( updateContext );
	}
	
	for( auto ecIt = m_effectChildren.begin(); ecIt != m_effectChildren.end(); ++ecIt ) 
	{
		(*ecIt)->UpdateAsyncronous( updateContext, this, nullptr );
	}

	if( m_impactOverlay )
	{
		m_impactOverlay->UpdateAsyncronous( updateContext, this );
	}
}

void EveSpaceObject2::PrepareShaderData( EveUpdateContext& updateContext ) 
{
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

	// if we have an impact overlay it can modify the activation strength, otherwise just full on
	m_spaceObjectShipData.y = m_impactOverlay ? m_impactOverlay->GetActivationStrength( updateContext ) : 1.f;

	// shader needs to know size of this object for some surface-scaling issues
	m_spaceObjectShipData.w = GetBoundingSphereRadius();
	// dirt level of a spaceobject
	m_spaceObjectShipData.z = m_dirtLevel;
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

	if( m_debugShowDamageLocators )
	{
		for( unsigned i = 0; i < m_persistedDamageLocators.size(); i++ )
		{
			Vector3 pos;
			GetDamageLocatorPosition( &pos, i, true );
			Tr2Renderer::DrawSphere( pos, m_boundingSphereRadius / 50.f, 4, 0xffff00ff );
			Vector3 dir;
			if( GetDamageLocatorDirection( &dir, i, true ) )
			{
				Tr2Renderer::DrawLine( pos, pos + (dir * m_boundingSphereRadius / 20.f), 0xffff00ff );
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

		for( auto it = m_spotlightSets.begin(); it != m_spotlightSets.end(); ++it )
		{
			(*it)->GetBatches( batches, perObjectData );
		}

		for( auto it = m_planeSets.begin(); it != m_planeSets.end(); ++it )
		{
			(*it)->GetBatches( batches, perObjectData );
		}
	}

	if( m_impactOverlay )
	{
		m_impactOverlay->GetBatches( batches, batchType, perObjectData );
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

	// first the damage overlays
	if( m_impactOverlay )
	{
		Tr2EffectPtr effect = m_impactOverlay->GetArmorDamageShader( batchType );
		if( effect )
		{
			for( auto areaBlock = m_overlayMeshAreaBlocks[EveMeshOverlayEffect::TYPE_ALL].begin(); areaBlock != m_overlayMeshAreaBlocks[EveMeshOverlayEffect::TYPE_ALL].end(); ++areaBlock )
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

	// second the effects
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
	memset( m_psData.shLightingCoefficients, 0, sizeof( m_psData.shLightingCoefficients ) );
	if( m_estimatedPixelDiameterWithChildren > g_eveSpaceSceneLowDetailThreshold )
	{
		float intensityFadeRadius = ( g_eveSpaceSceneMediumDetailThreshold - g_eveSpaceSceneLowDetailThreshold ) * 0.25f;
		float intensity = ( m_estimatedPixelDiameterWithChildren - g_eveSpaceSceneLowDetailThreshold ) / intensityFadeRadius;
		intensity = std::min( std::max( intensity, 0.f ), 1.f );
		manager.GetLighting( m_worldPosition, intensity, m_boundingSphereRadius * g_secondaryLightingRadiusCutoffFactor, m_psData.shLightingCoefficients );
	}
}

void EveSpaceObject2::ClearShLighting()
{
	memset( m_psData.shLightingCoefficients, 0, sizeof( m_psData.shLightingCoefficients ) );
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
	Tr2PerObjectDataWithPersistentBuffers<EveSpaceObject2>* perObjectData = accumulator->Allocate<Tr2PerObjectDataWithPersistentBuffers<EveSpaceObject2>>();
	if( !perObjectData )
	{
		return NULL;
	}
	perObjectData->Initialize( this, &m_perObjectDataVs, &m_perObjectDataPs );

	return perObjectData;
}

uint32_t EveSpaceObject2::GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const
{
	if( shaderType == Tr2RenderContextEnum::PIXEL_SHADER )
	{
		uint32_t sz = sizeof( m_psData ); // m_spaceObjectMiscData + m_spaceObjectClipData + m_spaceObjectClipDataEx
		if( m_customMask )
		{
			sz += 64 + 16 + 16;  // customMask data is optional for now
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
			sizeof( m_vsData ) +
			boneCount * 3 * 16;	// m_vsBonesMatrix (3x4)
	}
}

void EveSpaceObject2::UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* data )
{
	m_spaceObjectShipData.w = GetBoundingSphereRadius();

	if( shaderType == Tr2RenderContextEnum::PIXEL_SHADER )
	{
		uint8_t* perObjectPS = (uint8_t*)data;

		memcpy( perObjectPS, &m_psData, sizeof( m_psData ) );
		perObjectPS += sizeof( m_psData );

		if( m_customMask )
		{
			m_customMask->GetInvCustomMaskTransform( (Matrix*)perObjectPS );
			perObjectPS += sizeof( Matrix );
			m_customMask->GetMaterialID( (Vector4*)perObjectPS );
			perObjectPS += sizeof( Vector4 );
			m_customMask->GetExtendedData( (Vector4*)perObjectPS );
			perObjectPS += sizeof( Vector4 );
		}
	}
	else
	{
		uint8_t* perObjectVS = (uint8_t*)data;
		memcpy( perObjectVS, &m_vsData, sizeof( m_vsData ) );
		perObjectVS += sizeof( m_vsData );

		// maybe animated?
		size -= sizeof( m_vsData );
		if( size )
		{
			memcpy( perObjectVS, m_animationUpdater->GetMeshBoneMatrixList(), size );
		}
	}
}

void EveSpaceObject2::GetPerObjectStructs( EveSpaceObjectVSData& vsData, EveSpaceObjectPSData& psData ) const
{
	vsData = m_vsData;
	psData = m_psData;
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


// Actually submit renderables to the list, called from GetRenderables
void EveSpaceObject2::PushRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables )
{
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
		for( auto ecIt = m_effectChildren.begin(); ecIt != m_effectChildren.end(); ++ecIt )
		{
			(*ecIt)->GetRenderables( frustum, renderables, m_worldTransform, m_lodLevelWithChildren );
		}
	}

	// are decals visible?
	if( DisplayDecals() && m_mesh && m_isMeshVisible )
	{
		TriGeometryResPtr geometryRes = m_mesh->GetGeometryResource();
		if( geometryRes )
		{
			// put together parent data for the decals
			EveSpaceObjectDecal::ParentData pd;
			FillDecalParentData( &pd );

			// runn over every decal and update it
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
				(*it)->GetRenderables( geometryRes, frustum, renderables, &pd );
			}
			m_decalCache->Clear();
		}
	}
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

		PushRenderables( frustum, renderables );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   This funstion fills all the relevant data to pass to each decal as "parent's
//   data. With it being virtual, sub classes can add more data.
// Arguments:
//   pd - the data buffer
// --------------------------------------------------------------------------------
void EveSpaceObject2::FillDecalParentData( EveSpaceObjectDecal::ParentData* pd ) const
{
	memset( pd, 0, sizeof( EveSpaceObjectDecal::ParentData ) );
	pd->transform = m_worldTransform;
	pd->shipData = m_spaceObjectShipData;
	pd->clipData = m_psData.clipData;
	pd->clipDataEx = m_psData.miscData;
	pd->shLighting = m_psData.shLightingCoefficients;
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
	for( auto it = m_spriteLineSets.begin(); it != m_spriteLineSets.end(); ++it )
	{
		( *it )->RegisterWithQuadRenderer( quadRenderer );
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
	if( !m_isInFrustum || !m_display )
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
		(*it)->AddToQuadRenderer( quadRenderer, m_worldTransform, m_spaceObjectShipData.y, bones, boneCount );
	}
	for( auto it = m_spotlightSets.begin(); it != m_spotlightSets.end(); ++it )
	{
		(*it)->AddToQuadRenderer( quadRenderer, m_worldTransform, m_spaceObjectShipData.y, m_spaceObjectShipData.x, bones, boneCount );
	}
	for( auto it = m_spriteLineSets.begin(); it != m_spriteLineSets.end(); ++it )
	{
		( *it )->AddToQuadRenderer( quadRenderer, m_worldTransform, m_spaceObjectShipData.y, bones, boneCount );
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

// --------------------------------------------------------------------------------
// Description:
//   Access to the bounding sphere radius, but make sure we use the dynamic one
//   if this object has one!
// --------------------------------------------------------------------------------
float EveSpaceObject2::GetBoundingSphereRadius() const
{
	if( m_dynamicBoundingSphere.w != -1 )
	{
		return m_modelScale * m_dynamicBoundingSphere.w;
	}
	return m_modelScale * m_boundingSphereRadius;
}

// --------------------------------------------------------------------------------
// Description:
//   Access to the bounding sphere center, but make sure we use the dynamic one
//   if this object has one!
// --------------------------------------------------------------------------------
Vector3 EveSpaceObject2::GetBoundingSphereCenter( ) const
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

	if( m_animationSequencer )
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
	if( IsMatch( val, m_animationSequencer ) && m_animationSequencer )
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

bool EveSpaceObject2::IsAnimated() const 
{
	if( !m_shadowEffect )
	{
		CCP_LOGWARN( "EveSpaceObject2::IsAnimated No shadow effect found for %s, returning false", m_name.c_str() );
		return false;
	}

	// I don't like this, but we will need to do this while we don't have everything in the SOF 
	// (because the isSkinned parameter is stored in the sofHull) - Oli
	return StringFind( m_shadowEffect->GetEffectPathName(), "skinned" );
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

bool EveSpaceObject2::IsDamageLocatorFacingPosition( uint32_t index, const Vector3& posInObjectSpace )
{
	Vector3 damageLocatorDir = GetObjectSpaceDamageLocatorDirection( index );
	Vector3 lengthOfPos = (Vector3)XMVector3LengthEst( posInObjectSpace );
	Vector3 lengthOfMovedPos = (Vector3)XMVector3LengthEst( posInObjectSpace - damageLocatorDir);
	
	if( XMVector3Less( lengthOfMovedPos, lengthOfPos ) )
	{
		return true;
	}
	return false;
}

int EveSpaceObject2::GetClosestDamageLocatorIndex( const Vector3* position )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	// Run a single pass on the transformed damage locator positions and select the closest
	float max = std::numeric_limits<float>::max();
	XMVECTOR closestLength = XMVectorSet( max, max, max, 0.0 );
	int closestIndex = -1;

	Vector3 posInObjectSpace = ( Vector3 )XMVector3Transform( *position, m_invWorldTransform );

	for( unsigned int i = 0; i < m_persistedDamageLocators.size(); ++i )
	{
		if( IsDamageLocatorFacingPosition( i, posInObjectSpace ) )
		{
			XMVECTOR distanceFromDamageLocator = GetObjectSpaceDamageLocatorPosition( i ) - Vector3( posInObjectSpace );
			distanceFromDamageLocator = XMVector3LengthEst( distanceFromDamageLocator );
			if ( XMVector3Less( distanceFromDamageLocator, closestLength ) )
			{
				closestIndex = i;
				closestLength = distanceFromDamageLocator;
			}
		}
	}
	
	return closestIndex;
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
	CCP_STATS_ZONE( __FUNCTION__ );
	
	float minDistance = FLT_MAX;
	float maxDistance = FLT_MIN;
	float bestDirectionFit = 0.0f;
		
	Vector3 posInObjectSpace = (Vector3) XMVector3Transform( position, m_invWorldTransform );

	for( unsigned i = 0; i < m_persistedDamageLocators.size(); ++i )
	{
		if( IsDamageLocatorFacingPosition( i, posInObjectSpace ) )
		{
			Vector3 damageLocatorPosition = GetObjectSpaceDamageLocatorPosition(i);
			Vector3 damageLocatorDirection = GetObjectSpaceDamageLocatorDirection(i);

			Vector3 v( XMVectorSubtract( damageLocatorPosition, posInObjectSpace ) );
			float length = D3DXVec3Length( &v );
			minDistance = min( minDistance, length );
			maxDistance = max( maxDistance, length );
			D3DXVec3Normalize( &v, &v );
			float directionFit = GetDirectionFit( (Vector3)damageLocatorDirection , v );
			bestDirectionFit = max( bestDirectionFit, directionFit );
		}
	}

	float desiredFit = TriRand() * ( 0.25f - ( 1.0f - bestDirectionFit ) ) + 0.75f;
	float bestFit = 1.0f;
	
	int bestLocator = GetClosestDamageLocatorIndex( &position );
	for( unsigned i = 0; i < m_persistedDamageLocators.size(); ++i )
	{
		if( IsDamageLocatorFacingPosition( i, posInObjectSpace ) )
		{
			Vector3 damageLocatorPosition = GetObjectSpaceDamageLocatorPosition(i);
			Vector3 damageLocatorDirection = GetObjectSpaceDamageLocatorDirection(i);

			Vector3 v( XMVectorSubtract( damageLocatorPosition, posInObjectSpace ) );
			float fitValue = GetDistanceFit( minDistance, maxDistance - minDistance, v );
			D3DXVec3Normalize( &v, &v );
			fitValue *= GetDirectionFit( damageLocatorDirection, v );
			if( std::abs( fitValue - desiredFit ) < bestFit )
			{
				bestFit = std::abs( fitValue - desiredFit );
				bestLocator = (int)i;
			}	
		}
	}

	return bestLocator;
}

float EveSpaceObject2::GetRadius() const
{
	return GetBoundingSphereRadius();
}

bool EveSpaceObject2::GetDamageLocatorPosition( Vector3* out, int index, bool inWorldSpace )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	if( ( index < 0 ) || ( index >= int( m_persistedDamageLocators.size() ) ) )
	{
		*out = inWorldSpace ? m_worldTransform.GetTranslation() : Vector3( 0.f, 0.f, 0.f );
		return false;
	}

	*out = inWorldSpace ? GetTransformedDamageLocator( index ) : GetObjectSpaceDamageLocatorPosition( index );
	
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Decide if an incoming laser/stretch effect should show its impact effect.
//   Depends on the shields.
// --------------------------------------------------------------------------------
bool EveSpaceObject2::HasImpactConfigurationShield() const
{
	return m_impactOverlay && ( m_impactOverlay->GetImpactConfiguration() == EveImpactOverlay::IMPACT_SHIELD );
}

// --------------------------------------------------------------------------------
// Description:
//   Try to find the specified locator set and return a pointer to it
// --------------------------------------------------------------------------------
const LocatorStructureList* EveSpaceObject2::GetLocatorsForSet( const char* setName ) const
{
	for( auto it = m_locatorSets.cbegin(); it != m_locatorSets.cend(); ++it )
	{
		if( (*it)->HasName( setName ) )
		{
			return (*it)->GetLocators();
		}
	}
	return nullptr;
}


void EveSpaceObject2::GetImpactPosition( Vector3& out, int damageLocatorIndex, const Vector3& direction )
{
	if( !HasImpactConfigurationShield() )
	{
		GetDamageLocatorPosition( &out, damageLocatorIndex, true );
		return;
	}

	Vector3 tgtPosWS( 0.f, 0.f, 0.f );
	GetDamageLocatorPosition( &tgtPosWS, damageLocatorIndex, true );

	// convert position and direction into object space
	Vector3 tgtPosOS, dirOS;
	D3DXVec3TransformCoord( &tgtPosOS, &tgtPosWS, &m_invWorldTransform );
	D3DXVec3TransformNormal( &dirOS, &direction, &m_invWorldTransform );
	
	Vector3 center, radii;
	GetShapeEllipsoid(center, radii);
	IntersectEllipsoidRay( out, center, radii, tgtPosOS, dirOS );
	D3DXVec3TransformCoord( &out, &out, &m_worldTransform );
}

bool EveSpaceObject2::GetDamageLocatorDirection( Vector3* out, int index, bool inWorldSpace )
{
	if( ( index < 0 ) || ( index >= int( m_persistedDamageLocators.size() ) ) )
	{
		*out = Vector3( 0.f, 1.f, 0.f );
		return false;
	}

	*out = inWorldSpace ? GetTransformedDamageLocatorDirection( index ) : GetObjectSpaceDamageLocatorDirection( index );

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

Vector3 EveSpaceObject2::GetModelWorldPosition() const
{
	return Vector3( m_boundingSphereWorld.x, m_boundingSphereWorld.y, m_boundingSphereWorld.z );
}

void EveSpaceObject2::GetWorldVelocity( Vector3& velocity ) const
{
	velocity = m_worldVelocity;
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
		GetDamageLocatorPosition( out, -1, true );
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
	if( m_ballPosition )
	{
		m_ballPosition->Update( &m_worldPosition, time );
		m_ballPosition->GetValueDotAt( &m_worldVelocity, time );
	}
	else
	{
		m_worldPosition = Vector3( 0.0f, 0.0f, 0.0f );
		m_worldVelocity = Vector3( 0.f, 0.f , 0.f );
	}

	if( m_ballRotation )
	{
		m_ballRotation->Update( &m_worldRotation, time );
	}
	else
	{
		m_worldRotation = Quaternion( 0.f, 0.f, 0.f, 1.f );
	}

	if( m_modelRotation )
	{
		Quaternion modelRotation;
		m_modelRotation->Update( &modelRotation, time );
		Quaternion rotation;
		D3DXQuaternionMultiply( &rotation, &modelRotation, &m_worldRotation);
		D3DXMatrixRotationQuaternion(&m_worldTransform, &rotation);
	}
	else
	{
		D3DXMatrixRotationQuaternion(&m_worldTransform, &m_worldRotation);
	}

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
	//is this done in a parent class/subclass anywhere else?
	D3DXMatrixInverse( &m_invWorldTransform, nullptr, &m_worldTransform );
}

bool EveSpaceObject2::IsShadowReceiveEnabled()
{
	return m_enableShadow && m_shadowEffect;
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
	
void EveSpaceObject2::GetLocalToWorldTransform( Matrix &transform ) const
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
//   Add a new spritelineset to this object from the outside
// --------------------------------------------------------------------------------
void EveSpaceObject2::AddSpriteLineSet( EveSpriteLineSetPtr newSpriteLineSet )
{
	m_spriteLineSets.Append( newSpriteLineSet->GetRawRoot() );
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
//   Add a new overlayEffect to the space object
// --------------------------------------------------------------------------------
void EveSpaceObject2::AddOverlayEffect( EveMeshOverlayEffect* newOverlayEffect )
{
	this->m_overlayEffects.Append( newOverlayEffect );
}

// --------------------------------------------------------------------------------
// Description:
//   Remove a specific overlayEffect from the space object
// --------------------------------------------------------------------------------
void EveSpaceObject2::RemoveOverlayEffect( EveMeshOverlayEffect* overlayEffectToRemove )
{	
	ssize_t index = m_overlayEffects.FindKey( overlayEffectToRemove );	
	m_overlayEffects.Remove( index );
}

// --------------------------------------------------------------------------------
// Description:
//   Set a whole new set of damage locators
// --------------------------------------------------------------------------------
void EveSpaceObject2::SetDamageLocators( const EveDamageLocator* damageLocators, size_t damageLocatorCount )
{
	// is a structured list, so we can copy this in one big block
	m_persistedDamageLocators.Resize( damageLocatorCount );
	memcpy( &m_persistedDamageLocators[0], damageLocators, damageLocatorCount * sizeof( EveDamageLocator ) );
}

// --------------------------------------------------------------------------------
// Description:
//   Add a whole new set of locators, id'ed by name
// --------------------------------------------------------------------------------
void EveSpaceObject2::AddLocatorSet( const char* name, const Locator* locators, size_t locatorCount )
{
	// make and setup a new set of locators
	EveLocatorSetsPtr newSet;
	newSet.CreateInstance();
	newSet->Set( name, locators, locatorCount );

	// add it to the list WITHOUT checking if this name already exists
	m_locatorSets.Append( newSet );
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
//   Add a child to the effectChildren list
// --------------------------------------------------------------------------------
void EveSpaceObject2::AddToEffectChildrenList( IEveSpaceObjectChildPtr child ) 
{
	m_effectChildren.Append( child->GetRootObject() );
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

// --------------------------------------------------------------------------------
// Description:
//   Set the impact ovleray of this guy
// --------------------------------------------------------------------------------
void EveSpaceObject2::SetImpactOverlay( EveImpactOverlayPtr overlay )
{
	m_impactOverlay = overlay;
}

// --------------------------------------------------------------------------------
// Description:
//   Set the impact damage state: how many percent are gone?
// --------------------------------------------------------------------------------
void EveSpaceObject2::SetImpactDamageState( float shield, float armor, float hull, bool doCreateArmorImpacts )
{
	if( m_impactOverlay )
	{
		m_impactOverlay->SetDamageState( shield, armor, hull, doCreateArmorImpacts );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Toggle an animation on the impacts: for boosters, hardeners, et.c
// --------------------------------------------------------------------------------
void EveSpaceObject2::SetImpactAnimation( const std::string& name, bool enable, float duration )
{
	if( m_impactOverlay )
	{
		m_impactOverlay->ToggleEffect( name, enable, duration );
	}
}

// -----------------------------------------------------------------------------
// Description:
//   Clear out all impact/damage effects on this ship
// -----------------------------------------------------------------------------
void EveSpaceObject2::ClearImpactDamage()
{
	if( m_impactOverlay )
	{
		m_impactOverlay->Clear();
	}
}

// -----------------------------------------------------------------------------
// Description:
//   Create an impact effect on this object
// -----------------------------------------------------------------------------
int EveSpaceObject2::CreateImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime, float size )
{
	if( m_impactOverlay )
	{
		if( m_lodLevel > TR2_LOD_LOW )
		{
			return m_impactOverlay->CreateImpact( damageLocatorIndex, direction, lifeTime, size );
		}
	}
	return -1;
}

// -----------------------------------------------------------------------------
// Description:
//   Create an impact effect on this object by getting the closest damage locator from the position
// -----------------------------------------------------------------------------
int EveSpaceObject2::CreateImpactFromPosition( const Vector3& position, const Vector3& direction, float lifeTime, float size )
{
	int closestDamageLocator = GetClosestDamageLocatorIndex( &position );
	return CreateImpact( closestDamageLocator, direction, lifeTime, size );
}


// -----------------------------------------------------------------------------
// Description:
//   Update the effect on this object
// -----------------------------------------------------------------------------
bool EveSpaceObject2::UpdateImpact( Vector3& out, const Vector3& direction, int impactIndex )
{
	if( m_impactOverlay )
	{
		return m_impactOverlay->UpdateImpact( out, direction, impactIndex );
	}
	return false;
}


unsigned int EveSpaceObject2::GetDamageLocatorCount() const 
{
	return (unsigned int)m_persistedDamageLocators.size();
}

Vector3 EveSpaceObject2::GetDamageLocator( uint32_t index ) const
{
	if( size_t( index ) > m_persistedDamageLocators.size() )
	{
		return Vector3( 0, 0, 0 );
	}
	return GetObjectSpaceDamageLocatorPosition( index );
}

Vector3 EveSpaceObject2::GetDamageLocatorDirection( uint32_t index ) const
{
	if( size_t( index ) > m_persistedDamageLocators.size() )
	{
		return Vector3( 0, 0, 0 );
	}
	return GetObjectSpaceDamageLocatorDirection( index );
}

// --------------------------------------------------------------------------------
// Description:
//   Returns the damage locator positionin worldspace
// --------------------------------------------------------------------------------
Vector3 EveSpaceObject2::GetTransformedDamageLocator( uint32_t index )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( size_t( index ) > m_persistedDamageLocators.size() )
	{
		return Vector3( 0, 0, 0 );
	}
			
	Vector3 transformedDamageLocator = GetObjectSpaceDamageLocatorPosition( index );
	transformedDamageLocator = XMVector3TransformCoord( transformedDamageLocator, m_worldTransform );
	
	return transformedDamageLocator;	
}

// --------------------------------------------------------------------------------
// Description:
//   Returns the damage locator position in objectspace
// --------------------------------------------------------------------------------
Vector3 EveSpaceObject2::GetObjectSpaceDamageLocatorPosition( uint32_t index ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );
	EveDamageLocator damageLocator = m_persistedDamageLocators[index];

	Vector3 damageLocatorPosition = damageLocator.m_position;
	// We're assuming for now that the bone 0 isn't animated for performance reasons.
	if( damageLocator.m_boneIndex <= 0 )
	{
		// damage locator is not attached to a bone, return the position
		return damageLocator.m_position;
	}
	
	// If the damage locator is animated we extract the bone matrix and apply it to the damage locator position
	if( m_animationUpdater && m_animationUpdater->IsInitialized() )
	{
		size_t boneCount = size_t( m_animationUpdater->GetMeshBoneCount() );
		if( boneCount )
		{
			const granny_matrix_3x4* bones = m_animationUpdater->GetMeshBoneMatrixList();
			Matrix boneTF;
			D3DXMatrixIdentity( &boneTF );
			TriMatrixCopyFrom3x4( &boneTF, &bones[ damageLocator.m_boneIndex ] );
			damageLocatorPosition = XMVector3TransformCoord( damageLocatorPosition, boneTF );
		}
	}

	return damageLocatorPosition;
}

// --------------------------------------------------------------------------------
// Description:
//   Returns the damage locator direction in object space
// --------------------------------------------------------------------------------
Vector3 EveSpaceObject2::GetObjectSpaceDamageLocatorDirection( uint32_t index ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );
	EveDamageLocator damageLocator = m_persistedDamageLocators[index];

	Quaternion quat;
	quat = damageLocator.m_impactDirection;
	Vector3 damagelocatorDirection = (Vector3)XMVector3Rotate( Vector3( 0.f, 1.f, 0.f ), quat );

	// We're assuming for now that the bone 0 isn't animated for performance reasons.
	if( damageLocator.m_boneIndex <= 0)
	{
		// damage locator is not attached to a bone, return the direction
		return damagelocatorDirection;
	}
	
	// If the damage locator is animated we extract the bone matrix and apply it to the damage locator direction
	size_t boneCount = 0;	
	if( m_animationUpdater && m_animationUpdater->IsInitialized() )
	{
		boneCount = size_t( m_animationUpdater->GetMeshBoneCount() );
		if( boneCount )
		{
			const granny_matrix_3x4* bones = m_animationUpdater->GetMeshBoneMatrixList();

			Matrix boneTF;
			D3DXMatrixIdentity( &boneTF );	
			TriMatrixCopyFrom3x4( &boneTF, &bones[ damageLocator.m_boneIndex ] );
			damagelocatorDirection = XMVector3TransformNormal( damagelocatorDirection, boneTF );
		}
	}

	return damagelocatorDirection;
}

// --------------------------------------------------------------------------------
// Description:
//   Returns the damage locator direction in worldspace
// --------------------------------------------------------------------------------
Vector3 EveSpaceObject2::GetTransformedDamageLocatorDirection( uint32_t index )
{
	if( size_t( index ) > m_persistedDamageLocators.size() )
	{
		return Vector3( 0, 1, 0 );
	}
	Vector3 transformedImpactDirection;
	transformedImpactDirection = GetObjectSpaceDamageLocatorDirection( index );
	transformedImpactDirection = XMVector3TransformNormal( transformedImpactDirection, m_worldTransform );
	return transformedImpactDirection;	
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
//   Get the shape ellipsoid. Try the user-authored data first (if set), then
//   fallback to the dynamiccally generated
// --------------------------------------------------------------------------------
void EveSpaceObject2::GetShapeEllipsoid( Vector3& center, Vector3& radius )
{
	// do we have static, hans-authored data?
	if( m_shapeEllipsoidRadius.x > 0.f )
	{
		center = m_shapeEllipsoidCenter;
		radius = m_shapeEllipsoidRadius;
		return;
	}

	// ok, let's calc it!
	Vector3 mn( -1.f, -1.f, -1.f ), mx( 1.f, 1.f, 1.f );
	GetLocalBoundingBox( mn, mx );
	radius = 0.5f * TRI_SQRT3 * ( mx - mn );
	center = mn + 0.5f * ( mx - mn );
}

// --------------------------------------------------------------------------------
// Description:
//   Set the user-authored shape ellipsoid. This will be used instead of the
//   dynamically generated
// --------------------------------------------------------------------------------
void EveSpaceObject2::SetShapeEllipsoid( const Vector3* center, const Vector3* radius )
{
	m_shapeEllipsoidCenter = *center;
	m_shapeEllipsoidRadius = *radius;
}

// --------------------------------------------------------------------------------
// Description:
//   Get the estimated onscreen pixel diameter. Warning: you will get a number
//   even when the object is not in the frustum
// --------------------------------------------------------------------------------
float EveSpaceObject2::GetEstimatedPixelDiameter() const
{
	return m_estimatedPixelDiameter;
}

// --------------------------------------------------------------------------------
// Description:
//   Is this object in the frustum
// --------------------------------------------------------------------------------
bool EveSpaceObject2::IsInFrustum() const
{
	return m_isInFrustum;
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
bool EveSpaceObject2::ExecuteAnimationStateCommand( const EveAnimationCommand& cmd, const std::map<std::string, float>& parameters )
{
	return false;
}

void EveSpaceObject2::SetMeshLod( Tr2MeshLod* mesh )
{
	m_meshLod = mesh;
	m_mesh.Unlock();
}

void EveSpaceObject2::GetLights( Tr2LightManager& lightManager ) const
{
	XMMATRIX worldTransform = m_worldTransform;
	for( auto it = std::begin( m_lights ); it != std::end( m_lights ); ++it )
	{
		lightManager.AddPointLight( 
			Vector3( XMVector3TransformCoord( (* it )->m_position, worldTransform ) ), 
			( *it )->m_radius, 
			( *it )->m_color );
	}
}

namespace
{
const uint16_t ATTACHMENT_TYPE_OFFSET = 12;
const uint16_t ATTACHMENT_INDEX_MASK = 0xfff;
const uint16_t ATTACHMENT_TYPE_SPRITE_SET = 1;
const uint16_t ATTACHMENT_TYPE_SPOTLIGHT_SET = 2;
const uint16_t ATTACHMENT_TYPE_PLANE_SET = 3;
}

IRoot* EveSpaceObject2::GetID( uint16_t areaID )
{
	auto areaType = areaID >> ATTACHMENT_TYPE_OFFSET;
	auto areaIndex = areaID & ATTACHMENT_INDEX_MASK;
	switch( areaType )
	{
	case ATTACHMENT_TYPE_SPRITE_SET:
		for( auto it = std::begin( m_spriteSets ); it != std::end( m_spriteSets ); ++it )
		{
			auto items = ( *it )->GetSprites();
			if( items->size() > size_t( areaIndex ) )
			{
				return items->GetAt( areaIndex );
			}
			areaIndex -= int( items->size() );
		}
		return GetRawRoot();
	case ATTACHMENT_TYPE_SPOTLIGHT_SET:
		for( auto it = std::begin( m_spotlightSets ); it != std::end( m_spotlightSets ); ++it )
		{
			auto items = ( *it )->GetSpotlightItems();
			if( items->size() > size_t( areaIndex ) )
			{
				return items->GetAt( areaIndex );
			}
			areaIndex -= int( items->size() );
		}
		return GetRawRoot();
	case ATTACHMENT_TYPE_PLANE_SET:
		for( auto it = std::begin( m_planeSets ); it != std::end( m_planeSets ); ++it )
		{
			auto items = ( *it )->GetPlanes();
			if( items->size() > size_t( areaIndex ) )
			{
				return items->GetAt( areaIndex );
			}
			areaIndex -= int( items->size() );
		}
		return GetRawRoot();
	default:
		return GetRawRoot();
	}
}

void EveSpaceObject2::GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData )
{
	if( ( pickTypes & PICK_TYPE_PICKING ) != 0 )
	{
		GetBatches( batches, TRIBATCHTYPE_PICKING, perObjectData );
	}
	if( ( pickTypes & PICK_TYPE_OPAQUE ) != 0 )
	{
		GetBatches( batches, TRIBATCHTYPE_OPAQUE, perObjectData );
	}
	if( ( pickTypes & PICK_TYPE_TRANSPARENT ) != 0 )
	{
		if( !m_mesh || m_mesh->IsHidden() )
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
	if( ( pickTypes & PICK_TYPE_ATTACHMENTS ) != 0 )
	{
		uint16_t areaIDOffset = ATTACHMENT_TYPE_SPRITE_SET << ATTACHMENT_TYPE_OFFSET;
		for( auto it = m_spriteSets.begin(); it != m_spriteSets.end(); ++it )
		{
			(*it)->GetPickingBatches( batches, areaIDOffset, perObjectData );
		}
		areaIDOffset = ATTACHMENT_TYPE_SPOTLIGHT_SET << ATTACHMENT_TYPE_OFFSET;
		for( auto it = m_spotlightSets.begin(); it != m_spotlightSets.end(); ++it )
		{
			(*it)->GetPickingBatches( batches, areaIDOffset, perObjectData );
		}
		areaIDOffset = ATTACHMENT_TYPE_PLANE_SET << ATTACHMENT_TYPE_OFFSET;
		for( auto it = m_planeSets.begin(); it != m_planeSets.end(); ++it )
		{
			(*it)->GetPickingBatches( batches, areaIDOffset, perObjectData );
		}
	}
}