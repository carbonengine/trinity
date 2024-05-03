////////////////////////////////////////////////////////////
//
//    Created:   2015
//    Copyright: CCP 2015
//
#include "StdAfx.h"
#include "EveSwarm.h"
#include "Eve/EveUpdateContext.h"
#include "Tr2MeshArea.h"
#include "Tr2MeshBase.h"
#include "TriFrustumOrtho.h"
#include "Shader/Tr2Effect.h"

#include "include/TriMath.h"
#include "Utilities/BoundingBox.h"
#include "Utilities/BoundingSphere.h"

#include "Eve/SpaceObject/Attachments/EveBoosterSet2.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpotlightSet.h"
#include "Eve/Turret/EveTurretSet.h"

extern float g_eveSpaceSceneLODFactor;

EveSwarmRenderable::EveSwarmRenderable( IRoot* lockobj ) :
PARENTLOCK( m_decals )
{
	memset( &m_psData, 0, sizeof( EveSpaceObjectPSData ) );
	memset( &m_vsData, 0, sizeof( EveSpaceObjectVSData ) );
}

EveSwarmRenderable::~EveSwarmRenderable()
{
	m_owner = nullptr;
	m_mesh = nullptr;
}

void EveSwarmRenderable::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	if( !m_mesh )
	{
		return;
	}
	
	Tr2MeshAreaVector* areas = m_mesh->GetAreas( batchType );
	// transparent needs sorted meshareas
	if( batchType != TRIBATCHTYPE_TRANSPARENT )
	{
		m_mesh->GetBatches( batches, areas, perObjectData );
	}
	else
	{
		GetSortedBatchesFromMeshAreaVector( areas, batches, perObjectData, m_mesh, std::numeric_limits<float>::max(), &m_worldTransform );
	}
}

float EveSwarmRenderable::GetSortValue()
{
	Vector3 d = Tr2Renderer::GetViewPosition() - m_worldTransform.GetTranslation();
	float distance = Length( d );
	return distance;
}

Tr2PerObjectData* EveSwarmRenderable::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	Tr2PerObjectDataWithPersistentBuffers<EveSwarmRenderable>* perObjectData = accumulator->Allocate<Tr2PerObjectDataWithPersistentBuffers<EveSwarmRenderable>>();
	if( !perObjectData )
	{
		return NULL;
	}
	perObjectData->Initialize( this, &m_perObjectDataVs, &m_perObjectDataPs );

	return perObjectData;
}

uint32_t EveSwarmRenderable::GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const
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

void EveSwarmRenderable::UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* data )
{
	if( shaderType == Tr2RenderContextEnum::PIXEL_SHADER )
	{
		uint8_t* perObjectPS = (uint8_t*)data;
		memcpy( perObjectPS, &m_psData, sizeof( m_psData ) );
	}
	else
	{
		uint8_t* perObjectVS = (uint8_t*)data;
		memcpy( perObjectVS, &m_vsData, sizeof( m_vsData ) );
	}
}

bool EveSwarmRenderable::HasTransparentBatches()
{
	if( m_mesh )
	{
		return !(m_mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT )->empty());
	}

	return false;
}

void EveSwarmRenderable::InitializeRenderable( EveSwarm* owner, Tr2MeshBase* mesh )
{
	m_mesh = mesh;
	m_owner = owner;
}

void EveSwarmRenderable::SetWorldTransform( const Matrix& transform )
{
	m_worldTransform = transform;
	m_vsData.worldTransformLast = m_vsData.worldTransform;
	m_vsData.worldTransform = Transpose( m_worldTransform );
	m_vsData.invWorldTransform = Inverse( m_worldTransform );
	
	m_perObjectDataVs.InvalidateBufferData();
	m_perObjectDataPs.InvalidateBufferData();
}

void EveSwarmRenderable::SetBoosterIntensity( float intensity )
{
	m_psData.shipData.x = intensity;
}

void EveSwarmRenderable::SetShaderData( const EveSpaceObjectVSData& vsData, const EveSpaceObjectPSData& psData )
{
	m_vsData.clipData = vsData.clipData;
	m_vsData.ellpsoidCenter = vsData.ellpsoidCenter;
	m_vsData.ellpsoidRadii = vsData.ellpsoidRadii;
	m_vsData.shipData = vsData.shipData;

	m_psData.clipData = psData.clipData;
	m_psData.miscData = psData.miscData;
	memcpy( (void*)&m_psData.shLightingCoefficients, (void*)&psData.shLightingCoefficients, sizeof( m_psData.shLightingCoefficients ) );
	m_psData.shipData.y = psData.shipData.y;
	m_psData.shipData.z = psData.shipData.z;
	m_psData.shipData.w = psData.shipData.w;
}

void EveSwarmRenderable::InitDecals( const PEveSpaceObjectDecalVector &decals )
{
	for (EveSpaceObjectDecalVector::const_iterator it = decals.begin(); it != decals.end(); ++it)
	{
		EveSpaceObjectDecalPtr decal;
		decal.CreateInstance();
		decal->CopyFrom( *it );
		
		m_decals.Append( decal->GetRawRoot() );
	}
}

void EveSwarmRenderable::PushDecals( std::vector<ITr2Renderable*>& renderables, float screensize )
{
	TriGeometryResPtr geometryRes = m_mesh->GetGeometryResource();

	if( geometryRes )
	{
		// run over every decal and update it
		for (EveSpaceObjectDecalVector::const_iterator it = m_decals.begin(); it != m_decals.end(); ++it)
		{
			// now prep to get the renderables
			( *it )->GetRenderables( renderables, geometryRes, screensize );
		}
	}
}

void EveSwarmRenderable::UpdateDecalVisibility( const TriFrustum& frustum, IEveSpaceObject2::ParentData &pd, Tr2GrannyAnimation* animationUpdater )
{
	TriGeometryResPtr geometryRes = m_mesh->GetGeometryResource();

	if( geometryRes )
	{
		pd.transform = m_worldTransform;

		// run over every decal and update it
		for( EveSpaceObjectDecalVector::const_iterator it = m_decals.begin(); it != m_decals.end(); ++it )
		{
			// tell the decal of animation, IF we have any
			if( animationUpdater && animationUpdater->GetMeshBoneCount() && animationUpdater->IsInitialized() )
			{
				( *it )->SetBoneMatrix( animationUpdater->GetMeshBoneMatrixList(), animationUpdater->GetMeshBoneCount() );
			}
			// now prep to get the renderables
			( *it )->UpdateVisibility( frustum, &pd );
		}
	}
}

// --------------------------------------------------------------------------
IRoot* EveSwarmRenderable::GetID( uint16_t )
{
	if( !m_owner )
	{
		return nullptr;
	}
	return m_owner->GetRawRoot();
}

// --------------------------------------------------------------------------
void EveSwarmRenderable::GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData )
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
		GetBatches( batches, TRIBATCHTYPE_TRANSPARENT, perObjectData );
		GetBatches( batches, TRIBATCHTYPE_ADDITIVE, perObjectData );
	}
}

void EveSwarmRenderable::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	if( nullptr != m_mesh )
	{
		m_mesh->SetShaderOption( name, value );
	}
}

//////////////////////////////////////////////////////////////////////////////////////
// IEveShadowCaster
bool EveSwarmRenderable::IsCastingShadow( const TriFrustum& cameraFrustum, const TriFrustumOrtho& shadowFrustum, const uint32_t shadowMapSize, const Vector3 sunDir, float& sizeInShadow ) const
{
	Vector4 boundingSphere;
	if( !m_owner )
	{
		return false;
	}

	if( m_owner->GetBoundingSphere( boundingSphere ) )
	{
		boundingSphere.GetXYZ() = m_worldTransform.GetTranslation();
		sizeInShadow = 0;

		if( EveShadowCaster::IsVisible( cameraFrustum, shadowFrustum, sunDir, boundingSphere ) )
		{
			sizeInShadow = EveShadowCaster::GetSizeInShadow( shadowFrustum, shadowMapSize, boundingSphere );
		}
		return sizeInShadow > 15.f;
	}
	return false;
}

void EveSwarmRenderable::GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData, float shadowPixelSize )
{
	if( !m_mesh || !m_mesh->GetDisplay() )
	{
		return;
	}

	Tr2MeshAreaVector* areas = m_mesh->GetAreas( TRIBATCHTYPE_OPAQUE );

	TriGeometryRes* geomRes = m_mesh->GetGeometryResource();
	int meshIx = m_mesh->GetMeshIndex();

	for( Tr2MeshAreaVector::iterator it = areas->begin(); it != areas->end(); ++it )
	{
		Tr2MeshArea* area = *it;
		auto material = area->GetMaterialInterface();

		if( !area->GetDisplay() || !material )
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

Tr2PerObjectData* EveSwarmRenderable::GetShadowPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	return GetPerObjectData( accumulator );
}


void EveSwarmRenderable::RegisterComponents()
{
	auto reg = GetComponentRegistry();
	if( reg )
	{
		reg->RegisterComponent<IEveShadowCaster>( this );
	}
}

EveSwarm::EveSwarm( IRoot* lockobj ) :
	PARENTLOCK( m_renderables ),
	m_squadBoundsMax( 0, 0, 0 ),
	m_squadBoundsMin( 0, 0, 0 ),
	m_started( false ),
	m_targetIndex( 0 ),
	m_firingIndex( 0 ),
	m_worldAcceleration( 0, 0, 0 ),

	m_origin( UNINITIALIZED_ORIGIN, UNINITIALIZED_ORIGIN, UNINITIALIZED_ORIGIN ),
	m_timeLast( 0 ),
	m_lodUpdateTime( 1.f ),
	m_timeSinceUpdate( 0.f ),

	m_swarmingEnabled( false ),
	m_debugSize( 24.f ),
	m_count( 1 ),

	m_debugShowForces( false )
{
	// Stagger update time a little to avoid all fighters updating at the same time
	m_lodUpdateTime = 1.1f - TriRand() * 0.2f;
}

EveSwarm::~EveSwarm()
{
}

// --------------------------------------------------------------------------------
// Description:
//   Return a transform used for audio observers
// --------------------------------------------------------------------------------
Matrix EveSwarm::GetObserverTransform()
{
	Vector3 translation = GetModelWorldPosition() - m_worldPosition;
	Matrix translationMatrix = TranslationMatrix( translation );
	return m_worldTransform * translationMatrix;
}

// --------------------------------------------------------------------------------
// Description:
//   Return a transform used for turret locators
// --------------------------------------------------------------------------------
const Matrix* EveSwarm::GetTurretTransform( unsigned int swarmID ) const
{
	if( swarmID >= m_renderables.size() )
	{
		return &m_worldTransform;
	}
	return m_renderables[swarmID]->GetWorldTransform();
}

// --------------------------------------------------------------------------------
// Description:
//   From EveShip2
// --------------------------------------------------------------------------------
void EveSwarm::RebuildCachedData( BlueAsyncRes* p )
{
	EveShip2::RebuildCachedData( p );
	for( auto it = m_renderables.begin(); it != m_renderables.end(); ++it )
	{
		(*it)->InitializeRenderable( this, m_mesh );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   From EveShip2
// --------------------------------------------------------------------------------
void EveSwarm::UpdateSyncronous( EveUpdateContext& updateContext )
{
	if( m_swarmingEnabled )
	{
		UpdateSwarm( updateContext.GetTime() );
	}
	EveShip2::UpdateSyncronous( updateContext );
}

// --------------------------------------------------------------------------------
// Description:
//   From EveShip2
// --------------------------------------------------------------------------------
void EveSwarm::UpdateWorldTransform( Be::Time time )
{
	Vector3 velocityLast = m_worldVelocity;
	EveShip2::UpdateWorldTransform( time );
	m_worldAcceleration = m_worldVelocity - velocityLast;
}

// --------------------------------------------------------------------------------
// Description:
//   Calculate and set the vehcile's rotation
// --------------------------------------------------------------------------------
void EveSwarm::UpdateOrientation( SwarmVehicle* vehicle, float timeDiff )
{
	Vector3 frontDir( 0, 0, 1 ), upDir( 0, 1, 0 );
	Vector3 dir = Normalize( vehicle->velocity );
	Vector3 side = Cross( dir, upDir );
	float yaw = atan2( dir.x, dir.z );
	float pitch = asin( -dir.y );
	float roll = 0;
	float speed = Length( vehicle->velocity );
	if( speed > 0 )
	{
		// Roll is based on how large acceleration is in the direction of the side vector
		roll = 0.8f * Dot( vehicle->acceleration, side ) * TRI_PI / speed;
	}
	Quaternion rotation = RotationQuaternion( yaw, pitch, roll );
	vehicle->rotation = Slerp( vehicle->rotation, rotation, timeDiff * m_behavior.m_agility );
}


void EveSwarm::UpdateTurretsAsyncronous( EveUpdateContext& updateContext )
{
	for( EveTurretSetVector::iterator it = m_turretSets.begin(); it != m_turretSets.end(); ++it )
	{
		IEveSpaceObject2::ParentData pd;
		memset( &pd, 0, sizeof( ParentData ) );

		pd.transform = *GetTurretTransform( (*it)->GetSwarmID() );
		pd.shipData = m_spaceObjectShipData;
		pd.clipData = m_psData.clipData;
		(*it)->UpdateAsyncronous( updateContext, &pd );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   From EveShip2
// --------------------------------------------------------------------------------
void EveSwarm::UpdateAsyncronous( EveUpdateContext& context )
{
	if( !m_swarmingEnabled || m_count == 0 )
	{
		m_squadBoundsMax = Vector3( 0, 0, 0 );
		m_squadBoundsMin = Vector3( 0, 0, 0 );
		if( !m_renderables.empty() )
		{
			m_renderables[0]->SetWorldTransform( m_worldTransform );
			EveShip2::UpdateBoosters( context );
			if( m_boosters )
			{
				m_renderables[0]->SetBoosterIntensity( m_boosters->GetBoosterIntensity() );
			}
			m_renderables[0]->SetShaderData( m_vsData, m_psData );
		}
	}
	else
	{
		// Update world transforms
		auto rit = m_renderables.begin();
		for( unsigned i = 0; i < m_vehicles.size() && rit != m_renderables.end(); i++, rit++ )
		{
			Matrix world = RotationMatrix( m_vehicles[i].rotation ) * TranslationMatrix( m_vehicles[i].position );
			(*rit)->SetWorldTransform( world );
		
			if( m_boosters )
			{
				Be::Time time = context.GetTime();
				float deltaT = context.GetDeltaT();
				float speed = Length( m_vehicles[i].velocity );
				m_boosters->Update( deltaT, time, world, speed, m_vehicles[i].acceleration, m_vehicles[i].rotation, i );
				(*rit)->SetBoosterIntensity( m_boosters->GetBoosterIntensity() );
			}
			(*rit)->SetShaderData( m_vsData, m_psData );
		}
		if( m_boosters )
		{
			Be::Time time = context.GetTime();
			float deltaT = context.GetDeltaT();
			m_boosters->UpdateTrails( deltaT, time );
		}
	}

	EveShip2::UpdateAsyncronous( context );
}


void EveSwarm::UpdateSwarm( Be::Time t )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	if( t == m_timeLast )
	{
		return;
	}
	if( !m_timeLast )
	{
		m_timeLast = t;
	}
	
	Vector3 worldTransformLast = m_worldPosition;
	UpdateWorldTransform( t );

	if( !m_started )
	{
		// Set initial position
		for( unsigned i = 0; i < m_vehicles.size(); i++ )
		{
			m_vehicles[i].position = m_worldPosition;
		}
	}
	
	float timeDelta = TimeAsFloat( t - m_timeLast );
	float timeSeconds = TimeAsFloat( t - m_timeLast ) * m_behavior.m_timeMultiplier;
	if( timeSeconds > m_behavior.m_maxTime )
	{
		timeSeconds = m_behavior.m_maxTime;
	}
	
	m_timeSinceUpdate += timeDelta;
	m_timeLast = t;
	
	bool updateNow = m_isVisible || m_timeSinceUpdate >= m_lodUpdateTime;
	if( !updateNow )
	{
		if( m_started )
		{
			Vector3 movement = m_worldPosition - worldTransformLast;
			// Update velocities and positions
			for( unsigned i = 0; i < m_vehicles.size(); i++ )
			{
				m_vehicles[i].position += movement;
			}
			m_squadBoundsMax += movement;
			m_squadBoundsMin += movement;
		}
		m_started = true;
		return;
	}
	m_timeSinceUpdate = 0;

	Vector3 originShift( 0.f, 0.f, 0.f );
	Vector3d originNow( 0.0, 0.0, 0.0 );
	IEveReferencePointPtr refObject( BlueCastPtr( m_ballPosition ) );
	if( refObject )
	{
		refObject->GetReferencePoint( &originNow, t );
		if( m_origin.x != UNINITIALIZED_ORIGIN )
		{
			m_origin = m_origin - originNow;
			originShift = m_origin.AsVector3();
		}
		m_origin = originNow;
	}

	if( m_started )
	{
		for( unsigned i = 0; i < m_vehicles.size(); i++ )
		{
			m_vehicles[i].position += originShift;
		}
	}
	else
	{
		m_started = true;
	}
	
	Vector3 center( 0, 0, 0 );
	Vector3 alignment( 0, 0, 0 );
	if( updateNow )
	{
		BoundingBoxInitialize( m_squadBoundsMin, m_squadBoundsMax );
		// Calculate average velocity direction(alignment and center position(pre update)
		for( unsigned i = 0; i < m_vehicles.size(); i++ )
		{
			center += m_vehicles[i].position;
			alignment += m_vehicles[i].velocity;
		}
		if( m_vehicles.size() > 0 )
		{
			center *= 1.f / (float)m_vehicles.size();
			alignment = Normalize( alignment );
		}

		// Max speed is based of the ball speed + a minimum allowed speed
		float maxSpeed = m_behavior.m_speedMinimum;
		if( m_speed )
		{
			maxSpeed += m_behavior.m_speedMultiplier * m_speed->m_value;
		}
		float maxAcceleration = maxSpeed;

		// Calculate formation directions
		Vector3 formationDirection = Normalize( m_vehicles[0].velocity );
		Vector3 formationSide = Cross( formationDirection, Vector3( 0, 1, 0 ) );

		Vector3 followPosition = m_worldPosition + ( m_worldVelocity + m_worldAcceleration * timeDelta ) * timeDelta;
		// Calculate forces and acceleration
		for( unsigned i = 0; i < m_vehicles.size(); i++ )
		{
			Vector3 force = CalculateForces( i, m_vehicles, followPosition, center, alignment, formationDirection, formationSide, timeSeconds );
			Vector3 acc = force * 1.f / m_behavior.m_mass;
			m_vehicles[i].acceleration = acc;
			m_vehicles[i].acceleration = ClampLength( m_vehicles[i].acceleration, maxAcceleration );
		}

		// Update velocities and positions
		for( unsigned i = 0; i < m_vehicles.size(); i++ )
		{
			m_vehicles[i].velocity = m_vehicles[i].velocity + m_vehicles[i].acceleration * timeSeconds;
			m_vehicles[i].velocity = ClampLength( m_vehicles[i].velocity, maxSpeed );
			m_vehicles[i].position += m_vehicles[i].velocity * timeSeconds;
			UpdateOrientation( &m_vehicles[i], timeSeconds );
			BoundingBoxUpdate( m_squadBoundsMin, m_squadBoundsMax, m_vehicles[i].position );
		}
	}

	// Never let the center of the squadron get more than m_maxDistance from the world position(client hangs for while f.x.)
	center = 0.5f * (m_squadBoundsMin + m_squadBoundsMax);
	Vector3 d = m_worldPosition - center;
	float distance = Length( d );
	float maxDistance = Lerp( m_behavior.m_maxDistance0, m_behavior.m_maxDistance1, TriLinearize( m_behavior.m_speed0, m_behavior.m_speed1, Length( m_worldVelocity ) ) );
	if( distance > maxDistance )
	{
		// Move the center of the squad to maxDistance away from the world position
		d = Normalize( d );
		d *= distance - maxDistance;
		for( unsigned i = 0; i < m_vehicles.size(); i++ )
		{
			m_vehicles[i].position += d;
		}
		m_squadBoundsMax += d;
		m_squadBoundsMin += d;
	}
}
// --------------------------------------------------------------------------------
// Description:
//   Registers space object attachments (sprite and spotlight sets) with quad 
//   renderer.
// Arguments:
//   quadRenderer - quad renderer
// --------------------------------------------------------------------------------
void EveSwarm::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	for( auto it = m_attachments.begin(); it != m_attachments.end(); ++it )
	{
		(*it)->RegisterWithQuadRenderer( quadRenderer );
	}
	if( m_boosters )
	{
		m_boosters->RegisterWithQuadRenderer( quadRenderer );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Adds sprites from sprite sets and spotlight sets to quad renderer.
// Arguments:
//   quadRenderer - quad renderer
// --------------------------------------------------------------------------------
void EveSwarm::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer )
{
	if( !m_isInFrustum || !m_display )
	{
		return;
	}

	for( auto rit = m_renderables.begin(); rit != m_renderables.end(); ++rit )
	{
		for( auto it = m_attachments.begin(); it != m_attachments.end(); ++it )
		{
			(*it)->AddToQuadRenderer( quadRenderer, *(*rit)->GetWorldTransform(), 1, 1, nullptr, 0 );
		}
	}
	if( DisplayBoosters() )
	{
		m_boosters->AddToQuadRenderer( quadRenderer, IdentityMatrix() );
	}
}

void EveSwarm::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	EveShip2::GetDebugOptions( options );
	options.insert( "Vehicles" );
	options.insert( "Forces" );
	options.insert( "Swarm Bounds" );
}

// --------------------------------------------------------------------------------
// Description:
//   From EveShip2
// --------------------------------------------------------------------------------
void EveSwarm::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	EveShip2::RenderDebugInfo( renderer );

	for( unsigned i = 0; i < m_vehicles.size(); i++ )
	{
		Vector3 pos = m_vehicles[i].position;
		if( renderer.HasOption( this, "Vehicles" ) )
		{
			renderer.DrawSphere( this, pos, m_debugSize, 4, Tr2DebugRenderer::Wireframe, 0xffff00ff );
			renderer.DrawLine( this, pos, pos + m_vehicles[i].velocity, 0xffff00ff );
			renderer.DrawLine( this, pos, pos + m_vehicles[i].acceleration, 0xff0000ff );
		}
		
		if( renderer.HasOption( this, "Forces" ) && m_debugInfo.size() > i )
		{
			renderer.DrawLine( this, pos, pos + m_debugInfo[i].alignment, 0xff7f7f00 );
			renderer.DrawLine( this, pos, pos + m_debugInfo[i].anchor, 0xff007f7f );
			renderer.DrawLine( this, pos, pos + m_debugInfo[i].cohesion, 0xff00007f );
			renderer.DrawLine( this, pos, pos + m_debugInfo[i].separation, 0xff007f00 );
			renderer.DrawLine( this, pos, pos + m_debugInfo[i].wander, 0xff7f0000 );
			renderer.DrawLine( this, pos, pos + m_debugInfo[i].formation, 0xff7f7f7f );
		}
	}

	if( renderer.HasOption( this, "Swarm Bounds" ) )
	{
		Vector4 bs;
		Vector3 min, max;
		GetBoundingSphere( bs );
		GetLocalBoundingBox( min, max );
		renderer.DrawSphere( this, bs, 6, Tr2DebugRenderer::Wireframe, 0xffff00ff );
		renderer.DrawBox( this, min, max, Tr2DebugRenderer::Wireframe, 0xffff00ff );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Actually add renderables to the list of renderables after culling
// --------------------------------------------------------------------------------
void EveSwarm::PushRenderables( std::vector<ITr2Renderable*>& renderables )
{
	for( auto it = m_renderables.begin(); it != m_renderables.end(); it++ )
	{
		renderables.push_back( *it );
	}

	// are decals visible?
	if (m_mesh && m_isMeshVisible)
	{
		// put together parent data for the decals
		IEveSpaceObject2::ParentData pd;
		GetParentData( &pd );

		for (auto it = m_renderables.begin(); it != m_renderables.end(); it++)
		{
			( *it )->UpdateDecalVisibility( m_frustum, pd, m_animationUpdater );
			( *it )->PushDecals( renderables, m_estimatedPixelDiameter / g_eveSpaceSceneLODFactor );
		}
	}
}

void EveSwarm::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform )
{
	EveShip2::UpdateVisibility(
		frustum, parentTransform );

	m_frustum = frustum;
}


// --------------------------------------------------------------------------------
void EveSwarm::EstimatePixelDiameter( const TriFrustum& frustum )
{
	Vector4 bs;
	GetBoundingSphere( bs );
	m_estimatedPixelDiameter = frustum.GetPixelSizeAccross( &bs );
	if( m_swarmingEnabled && m_count > 1 )
	{
		m_estimatedPixelDiameter /= std::sqrt( (float)m_count );
	}
}


// --------------------------------------------------------------------------------
void EveSwarm::UpdateWorldBounds()
{
	Vector4 bs;
	BoundingSphereFromBox( bs, m_squadBoundsMin, m_squadBoundsMax );
	m_boundingSphereWorldCenter.x = bs.x;
	m_boundingSphereWorldCenter.y = bs.y;
	m_boundingSphereWorldCenter.z = bs.z;
	m_boundingSphereWorldRadius = bs.w + m_modelScale * m_boundingSphereRadius;
}

// --------------------------------------------------------------------------------
// Description:
//    GetBoundingSphere. See EveSpaceObject2
// --------------------------------------------------------------------------------
bool EveSwarm::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const 
{
	Vector4 s;
	EveShip2::GetBoundingSphere( s, query );
	BoundingSphereFromBox( sphere, m_squadBoundsMin, m_squadBoundsMax );
	sphere.w += s.w;
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//     This returns the last computed center world position. Will try to figure out
//     how to do this on demand.
// Original description: This version of the function should perform an update on the model / ball position
// --------------------------------------------------------------------------------
void EveSwarm::UpdateModelCenterWorldPosition( Vector3 &position, Be::Time t )
{
	if( m_swarmingEnabled )
	{
		UpdateSwarm( t );
		position = ( m_squadBoundsMax + m_squadBoundsMin ) * 0.5f;
	}
	else
	{
		EveShip2::UpdateModelCenterWorldPosition( position, t );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   From EveShip2
// --------------------------------------------------------------------------------
void EveSwarm::GetModelCenterWorldPosition( Vector3 &position ) const
{
	if( m_swarmingEnabled )
	{
		position = ( m_squadBoundsMax + m_squadBoundsMin ) * 0.5f;
	}
	else
	{
		EveShip2::GetModelCenterWorldPosition( position );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   From EveShip2
// --------------------------------------------------------------------------------
bool EveSwarm::GetLocalBoundingBox( Vector3 &min, Vector3 &max )
{
	if( m_mesh && m_mesh->GetBoundingBox( min, max ) )
	{
		Vector4 bs;
		BoundingSphereFromBox( bs, min, max );
		BoundingBoxUpdate( min, max, bs );
		min += m_squadBoundsMin;
		max += m_squadBoundsMax;
	}
	else
	{
		min = m_squadBoundsMin;
		max = m_squadBoundsMax;
	}
	if( m_swarmingEnabled )
	{
		m_localAabbMin = min - m_worldPosition;
		m_localAabbMax = max - m_worldPosition;
		min = m_localAabbMin;
		max = m_localAabbMax;
	}
	else
	{
		m_localAabbMin = min;
		m_localAabbMax = max;
	}
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   From EveShip2
// --------------------------------------------------------------------------------
bool EveSwarm::Initialize()
{
	EveShip2::Initialize();
	int count = m_count;
	m_count = 0;
	SetCount( count );
	EnableSwarmForceDebug( m_debugShowForces );
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   From EveShip2
// --------------------------------------------------------------------------------
bool EveSwarm::OnModified( Be::Var* val )
{
	EveShip2::OnModified( val );
	if( IsMatch( val, m_debugShowForces ) )
	{
		EnableSwarmForceDebug( m_debugShowForces );
	}
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Adds one swarmer to this swarm
// --------------------------------------------------------------------------------
void EveSwarm::AddSwarmer()
{
	auto componentRegistry = GetComponentRegistry();

	EveSwarmRenderablePtr renderable;
	renderable.CreateInstance();
	renderable->InitializeRenderable( this, m_mesh );
	renderable->InitDecals( m_decals );

	if( componentRegistry )
	{
		renderable->Register( componentRegistry );
	}

	m_renderables.Append( renderable->GetRawRoot() );
	SwarmVehicle v;
	v.position = m_worldPosition;
	m_vehicles.push_back( v );
	if( m_debugShowForces )
	{
		m_debugInfo.push_back( SwarmVehicleDebug() );
	}
	m_count++;
	
	if( m_boosters )
	{
		m_boosters->SetCount( m_count );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Removes one swarmer from this swarm
// --------------------------------------------------------------------------------
Vector3 EveSwarm::RemoveSwarmer()
{
	if( m_vehicles.empty() )
	{
		return Vector3(0, 0, 0);
	}

	SwarmVehicle v = m_vehicles[m_targetIndex];
	m_vehicles[m_targetIndex] = m_vehicles.back();
	m_vehicles.pop_back();
	m_renderables[m_targetIndex]->InitializeRenderable( nullptr, nullptr );

	auto componentRegistry = GetComponentRegistry();
	if( componentRegistry )
	{
		m_renderables[m_targetIndex]->UnRegister( componentRegistry );
	}

	m_renderables.Remove( m_targetIndex );
	if( m_debugShowForces )
	{
		m_debugInfo.pop_back();
	}
	m_count--;
	
	if( m_boosters )
	{
		m_boosters->SetCount( m_count );
	}
	m_targetIndex = TriRandInt( m_count );
	return v.position;
}

// --------------------------------------------------------------------------------
// Description:
//   Sets swarmer count
// --------------------------------------------------------------------------------
void EveSwarm::SetCount( int count )
{
	while( m_count != count )
	{
		if( m_count > count )
		{
			RemoveSwarmer();
		}
		else
		{
			AddSwarmer();
		}
		m_targetIndex = TriRandInt( m_count );
	}
}

void EveSwarm::RegisterComponents()
{
	EveShip2::RegisterComponents();
	auto reg = GetComponentRegistry();
	if( reg )
	{
		for( auto& renderable : m_renderables )
		{
			renderable->Register( reg );
		}
	}
}

void EveSwarm::UnRegisterComponents()
{
	EveShip2::UnRegisterComponents();
	auto reg = GetComponentRegistry();
	if( reg )
	{
		for( auto& renderable : m_renderables )
		{
			renderable->UnRegister( reg );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Toggle swarm behavior. If disabled swarm count is set to 1 and the swarmer
//   does not use the behavior but acts like a 'normal' space object.
// --------------------------------------------------------------------------------
void EveSwarm::EnableSwarming( bool enable )
{
	if( m_swarmingEnabled == enable )
	{
		return;
	}

	m_swarmingEnabled = enable;
	if( !enable )
	{
		SetCount( 0 ); // Remove all
		SetCount( 1 ); // Add a single 'neutral' swarmer
		m_squadBoundsMin = Vector3( 0, 0, 0 );
		m_squadBoundsMax = Vector3( 0, 0, 0 );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Select which fighter is the origin of firing effects
// --------------------------------------------------------------------------------
void EveSwarm::PickFiringOrigin()
{
	m_firingIndex = TriRandInt( m_count );
}

// --------------------------------------------------------------------------------
// Description:
//   Keep track of swarm forces for debug rendering
// --------------------------------------------------------------------------------
void EveSwarm::EnableSwarmForceDebug( bool enable )
{
	m_debugInfo.clear();

	if( !enable )
	{
		return;
	}
	for( unsigned i = 0; i < m_vehicles.size(); i++ )
	{
		m_debugInfo.push_back( SwarmVehicleDebug() );
	}
}

// --------------------------------------------------------------------------------
void EveSwarm::GetLocatorInObjectSpace( Vector3& position, Vector3& direction, const Locator& locator ) const
{
	EveShip2::GetLocatorInObjectSpace( position, direction, locator );
	if( m_count )
	{
		Matrix localTransform = *m_renderables[m_targetIndex]->GetWorldTransform() * m_invWorldTransform;
		position = TransformCoord( position, localTransform );
		direction = TransformNormal( direction, localTransform );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Override from EveSpaceObject2
// --------------------------------------------------------------------------------
bool EveSwarm::GetDamageLocatorPosition( Vector3* out, int index, bool inWorldSpace )
{
	if( !m_count )
	{
		return EveShip2::GetDamageLocatorPosition( out, index, inWorldSpace );
	}

	if( index >= 0 )
	{
		auto damageLocators = GetLocatorsForSet( BlueSharedString( "damage" ) );
		if( damageLocators && index < int( damageLocators->size() ) )
		{
			const Locator& damageLocator = ( *damageLocators )[index];

			Vector3 position, direction;
			GetLocatorInObjectSpace( position, direction, damageLocator );

			if( inWorldSpace )
			{
				*out = XMVector3TransformCoord( direction, m_worldTransform );
			}
			else
			{
				*out = position;
			}
			return true;
		}
	}

	if( inWorldSpace )
	{
		*out = m_renderables[m_targetIndex]->GetWorldTransform()->GetTranslation();
	}
	else
	{
		Matrix localTransform = *m_renderables[m_targetIndex]->GetWorldTransform() * m_invWorldTransform;
		*out = localTransform.GetTranslation();
	}
	return false;
}

// --------------------------------------------------------------------------------
// Description:
//   Calculate all forces influencing the swarm vehicle i0
// --------------------------------------------------------------------------------
Vector3 EveSwarm::CalculateForces( int i0, std::vector<SwarmVehicle>& swarmers, const Vector3& followPosition, const Vector3& centerOfMass, const Vector3& alignment, const Vector3& formationDirection, const Vector3& formationSide, float timeSeconds )
{
	Vector3 force( 0, 0, 0 ), separation( 0, 0, 0 );

	auto wander = m_behavior.m_weightWander * Calculate_Wander( swarmers[i0], m_behavior.m_wanderDistance, m_behavior.m_wanderRadius, m_behavior.m_wanderFluctuation, timeSeconds );
	auto cohesion = m_behavior.m_weightCohesion * Calculate_Cohesion( swarmers[i0].position, centerOfMass );
	auto anchor = Calculate_Cohesion( swarmers[i0].position, followPosition );
	auto anchorDistance = Length( anchor ); 
	anchorDistance = TriLinearize( m_behavior.m_anchorRadius0, m_behavior.m_anchorRadius1, anchorDistance );
	anchor = anchorDistance * m_behavior.m_weightAnchor * anchor;
	auto align = m_behavior.m_weightAlign * alignment;
	auto decelerate = swarmers[i0].velocity * -m_behavior.m_weightDecelerate;
	decelerate = ClampLength( decelerate, m_behavior.m_maxDeceleration );

	for( unsigned i = 0; i < swarmers.size(); i++ )
	{
		if( i0 != i )
		{
			separation += m_behavior.m_weightSeparation * Calculate_Separation( swarmers[i0].position, swarmers[i].position );
		}
	}

	// Formation
	Vector3 formationPosition = m_vehicles[0].position + formationDirection * m_behavior.m_formationDistance * (float)m_vehicles.size() * 0.25f;
	if( i0 && i0 & 1 )
	{
		float rankMultiplier = floor( 0.5f * static_cast<float>( i0 ) + 0.5f );
		formationPosition = formationPosition - formationDirection * m_behavior.m_formationDistance * rankMultiplier + formationSide * m_behavior.m_formationDistance * rankMultiplier * 0.5f;
	}
	else if( i0 )
	{
		float rankMultiplier = floor( 0.5f * static_cast<float>( i0 ) + 0.5f );
		formationPosition = formationPosition - formationDirection * m_behavior.m_formationDistance * rankMultiplier - formationSide * m_behavior.m_formationDistance * rankMultiplier * 0.5f;
	}
	auto formation = m_behavior.m_weightFormation * Calculate_Cohesion( swarmers[i0].position, formationPosition );
	
	// Debug info
	if( m_debugShowForces && m_debugInfo.size() > static_cast<unsigned>( i0 ) )
	{
		m_debugInfo[i0].alignment = align;
		m_debugInfo[i0].anchor = anchor;
		m_debugInfo[i0].cohesion = cohesion;
		m_debugInfo[i0].separation = separation;
		m_debugInfo[i0].wander = wander;
		m_debugInfo[i0].formation = formation;
		m_debugInfo[i0].deceleration = decelerate;
	}
	return wander + separation + align + cohesion + anchor + decelerate + formation;
}

// --------------------------------------------------------------------------------
// Description:
//   Cohesion. The vector from p0 to p1
// --------------------------------------------------------------------------------
inline Vector3 EveSwarm::Calculate_Cohesion( Vector3 p0, Vector3 p1 )
{
	return p1 - p0;
}

// --------------------------------------------------------------------------------
// Description:
//   Force that pushes p0 away from p1 depending on distance
// --------------------------------------------------------------------------------
inline Vector3 EveSwarm::Calculate_Separation( Vector3 p0, Vector3 p1 )
{
	Vector3 d = p0 - p1;
	float length = Length( d );
	if( length == 0.f )
	{
		return Vector3(TriRand() - 0.5f, TriRand() - 0.5f, TriRand() - 0.5f);
	}
	return Normalize( d ) * m_behavior.m_separationDistance / length;
}

// --------------------------------------------------------------------------------
// Description:
//   A random wandering behavior
// --------------------------------------------------------------------------------
Vector3 EveSwarm::Calculate_Wander( SwarmVehicle& s, float wanderDistance, float radius, float fluctuation, float t )
{
	// Evolve the target point on the 'sphere' around a point wanderDistance in front of our swarmer
	Vector3 target = s.wanderTarget;
	Vector3 newOffset = Normalize( Vector3( 2*TriRand() - 1.f, 2*TriRand() - 1.f, 2*TriRand() - 1.f ) );
	newOffset *= fluctuation * radius * t;
	target += newOffset;
	target = Normalize( target );
	target *= radius;
	s.wanderTarget = target;

	// And calculate the final target force
	if( LengthSq( s.velocity ) != 0 )
	{
		target = Normalize( s.velocity );
	}
	else
	{
		// start in the wander direction if no velocity
		target = Normalize( s.wanderTarget );
	}
	target = target * wanderDistance + s.wanderTarget;
	return target;
}
