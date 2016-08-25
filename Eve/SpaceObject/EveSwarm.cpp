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
#include "Tr2MeshLod.h"

#include "include/TriMath.h"
#include "Utilities/BoundingBox.h"
#include "Utilities/BoundingSphere.h"

#include "Eve/SpaceObject/Attachments/EveBoosterSet2.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpotlightSet.h"

EveSwarmRenderable::EveSwarmRenderable( IRoot* lockobj )
{
	memset( &m_psData, 0, sizeof( EveSpaceObjectPSData ) );
	memset( &m_vsData, 0, sizeof( EveSpaceObjectVSData ) );
}

EveSwarmRenderable::~EveSwarmRenderable()
{
}

void EveSwarmRenderable::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData )
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
		GetSortedBatchesFromMeshAreaVector( areas, batches, perObjectData, m_mesh, &m_worldTransform );
	}
}

void EveSwarmRenderable::GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData )
{
	if( m_mesh )
	{
		m_mesh->GetBatches( batches, m_mesh->GetAreas( TRIBATCHTYPE_OPAQUE ), perObjectData );
	}
}

float EveSwarmRenderable::GetSortValue()
{
	Vector3 d = Tr2Renderer::GetViewPosition() - m_worldTransform.GetTranslation();
	float distance = D3DXVec3Length( &d );
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

void EveSwarmRenderable::SetMesh( Tr2MeshBase* mesh )
{
	m_mesh = mesh;
}

void EveSwarmRenderable::SetWorldTransform( const Matrix& transform )
{
	m_worldTransform = transform;
	m_vsData.worldTransformLast = m_vsData.worldTransform;
	D3DXMatrixTranspose( &m_vsData.worldTransform, &m_worldTransform );
	
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

	m_debugShowSwarmBounds( true ),
	m_debugShowVehicle( true ),
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
Matrix EveSwarm::GetObserverTransform() const
{
	Vector3 translation = GetModelWorldPosition() - m_worldPosition;
	Matrix translationMatrix;
	D3DXMatrixTranslation( &translationMatrix, translation.x, translation.y, translation.z );
	return m_worldTransform * translationMatrix;
}

// --------------------------------------------------------------------------------
// Description:
//   Return a transform used for turret locators
// --------------------------------------------------------------------------------
const Matrix* EveSwarm::GetTurretTransform() const
{
	if( m_count < 1 || m_renderables.size() < 1 )
	{
		return &m_worldTransform;
	}
	if( (unsigned)m_firingIndex >= m_renderables.size() )
	{
		return m_renderables[0]->GetWorldTransform();
	}
	return m_renderables[m_firingIndex]->GetWorldTransform();
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
		(*it)->SetMesh( m_meshLod );
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
	Vector3 frontDir( 0, 0, 1 ), upDir( 0, 1, 0 ), dir, side;
	D3DXVec3Normalize( &dir, &vehicle->velocity );
	D3DXVec3Cross( &side, &dir, &upDir );
	float yaw = atan2( dir.x, dir.z );
	float pitch = asin( -dir.y );
	float roll = 0;
	float speed = D3DXVec3Length( &vehicle->velocity );
	if( speed > 0 )
	{
		// Roll is based on how large acceleration is in the direction of the side vector
		roll = 0.8f * D3DXVec3Dot( &vehicle->acceleration, &side ) * TRI_PI / speed;
	}
	Quaternion rotation;
	D3DXQuaternionRotationYawPitchRoll( &rotation, yaw, pitch, roll );
	D3DXQuaternionSlerp( &vehicle->rotation, &vehicle->rotation, &rotation, timeDiff * m_behavior.m_agility );
}

// --------------------------------------------------------------------------------
// Description:
//   From EveShip2
// --------------------------------------------------------------------------------
void EveSwarm::UpdateAsyncronous( EveUpdateContext& context )
{
	EveShip2::UpdateAsyncronous( context );

	if( !m_swarmingEnabled || m_count == 0 )
	{
		m_squadBoundsMax = Vector3( 0, 0, 0 );
		m_squadBoundsMin = Vector3( 0, 0, 0 );
		if( m_renderables.size() > 0 )
		{
			m_renderables[0]->SetWorldTransform( m_worldTransform );
			EveShip2::UpdateBoosters( context );
			if( m_boosters )
			{
				m_renderables[0]->SetBoosterIntensity( m_boosters->GetBoosterIntensity() );
			}
			m_renderables[0]->SetShaderData( m_vsData, m_psData );
		}
		return;
	}

	// Update world transforms
	auto rit = m_renderables.begin();
	for( unsigned i = 0; i < m_vehicles.size() && rit != m_renderables.end(); i++, rit++ )
	{
		Matrix world;
		D3DXMatrixAffineTransformation( &world, 1.f, nullptr, &(m_vehicles[i].rotation), &(m_vehicles[i].position) );
		(*rit)->SetWorldTransform( world );
		
		if( m_boosters )
		{
			Be::Time time = context.GetTime();
			float deltaT = context.GetDeltaT();
			float speed = D3DXVec3Length( &m_vehicles[i].velocity );
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
			D3DXVec3Normalize( &alignment, &alignment );
		}

		// Max speed is based of the ball speed + a minimum allowed speed
		float maxSpeed = m_behavior.m_speedMinimum;
		if( m_speed )
		{
			maxSpeed += m_behavior.m_speedMultiplier * m_speed->m_value;
		}
		float maxAcceleration = maxSpeed;

		// Calculate formation directions
		Vector3 formationSide, formationDirection, up( 0, 1, 0 );
		D3DXVec3Normalize( &formationDirection, &m_vehicles[0].velocity );
		D3DXVec3Cross( &formationSide, &formationDirection, &up );

		Vector3 followPosition = m_worldPosition + ( m_worldVelocity + m_worldAcceleration * timeDelta ) * timeDelta;
		// Calculate forces and acceleration
		for( unsigned i = 0; i < m_vehicles.size(); i++ )
		{
			Vector3 force = CalculateForces( i, m_vehicles, followPosition, center, alignment, formationDirection, formationSide, timeSeconds );
			Vector3 acc = force * 1.f / m_behavior.m_mass;
			m_vehicles[i].acceleration = acc;
			TriVectorClampLength( &m_vehicles[i].acceleration, maxAcceleration );
		}

		// Update velocities and positions
		for( unsigned i = 0; i < m_vehicles.size(); i++ )
		{
			m_vehicles[i].velocity = m_vehicles[i].velocity + m_vehicles[i].acceleration * timeSeconds;
			TriVectorClampLength( &m_vehicles[i].velocity, maxSpeed );
			m_vehicles[i].position += m_vehicles[i].velocity * timeSeconds;
			UpdateOrientation( &m_vehicles[i], timeSeconds );
			BoundingBoxUpdate( m_squadBoundsMin, m_squadBoundsMax, m_vehicles[i].position );
		}
	}

	// Never let the center of the squadron get more than m_maxDistance from the world position(client hangs for while f.x.)
	center = 0.5f * (m_squadBoundsMin + m_squadBoundsMax);
	Vector3 d = m_worldPosition - center;
	float distance = D3DXVec3Length( &d );
	float maxDistance = Lerp( m_behavior.m_maxDistance0, m_behavior.m_maxDistance1, TriLinearize( m_behavior.m_speed0, m_behavior.m_speed1, D3DXVec3Length( &m_worldVelocity ) ) );
	if( distance > maxDistance )
	{
		// Move the center of the squad to maxDistance away from the world position
		D3DXVec3Normalize( &d, &d );
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
	for( auto it = m_spriteSets.begin(); it != m_spriteSets.end(); ++it )
	{
		(*it)->UseQuadRenderer( true, false );
		(*it)->RegisterWithQuadRenderer( quadRenderer );
	}
	for( auto it = m_spotlightSets.begin(); it != m_spotlightSets.end(); ++it )
	{
		(*it)->UseQuadRenderer( true, false );
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
void EveSwarm::AddQuadsToQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	if( !m_isInFrustum || !m_display )
	{
		return;
	}

	for( auto rit = m_renderables.begin(); rit != m_renderables.end(); ++rit )
	{
		for( auto it = m_spriteSets.begin(); it != m_spriteSets.end(); ++it )
		{
			(*it)->AddToQuadRenderer( quadRenderer, *(*rit)->GetWorldTransform(), 1, nullptr, 0 );
		}
		for( auto it = m_spotlightSets.begin(); it != m_spotlightSets.end(); ++it )
		{
			(*it)->AddToQuadRenderer( quadRenderer, *(*rit)->GetWorldTransform(), 1, 1, nullptr, 0 );
		}
	}
	if( DisplayBoosters() )
	{
		m_boosters->AddToQuadRenderer( quadRenderer, Tr2Renderer::GetIdentityTransform() );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   From EveShip2
// --------------------------------------------------------------------------------
void EveSwarm::RenderDebugInfo( Tr2RenderContext& renderContext )
{
	EveShip2::RenderDebugInfo( renderContext );

	for( unsigned i = 0; i < m_vehicles.size(); i++ )
	{
		Vector3 pos = m_vehicles[i].position;
		if( m_debugShowVehicle )
		{
			Tr2Renderer::DrawSphere( pos, m_debugSize, 4, 0xffff00ff );
			Tr2Renderer::DrawLine( pos, pos + m_vehicles[i].velocity, 0xffff00ff );
			Tr2Renderer::DrawLine( pos, pos + m_vehicles[i].acceleration, 0xff0000ff );
		}
		
		if( m_debugShowForces && m_debugInfo.size() > i )
		{
			Tr2Renderer::DrawLine( pos, pos + m_debugInfo[i].alignment, 0xff7f7f00 );
			Tr2Renderer::DrawLine( pos, pos + m_debugInfo[i].anchor, 0xff007f7f );
			Tr2Renderer::DrawLine( pos, pos + m_debugInfo[i].cohesion, 0xff00007f );
			Tr2Renderer::DrawLine( pos, pos + m_debugInfo[i].separation, 0xff007f00 );
			Tr2Renderer::DrawLine( pos, pos + m_debugInfo[i].wander, 0xff7f0000 );
			Tr2Renderer::DrawLine( pos, pos + m_debugInfo[i].formation, 0xff7f7f7f );
		}
	}

	if( m_debugShowSwarmBounds )
	{
		Vector4 bs;
		Vector3 min, max;
		GetBoundingSphere( bs );
		GetLocalBoundingBox( min, max );
		Tr2Renderer::DrawSphere( bs, 6, 0xffff00ff );
		Tr2Renderer::DrawBox( min, max, 0xffff00ff );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Actually add renderables to the list of renderables after culling
// --------------------------------------------------------------------------------
void EveSwarm::PushRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables )
{
	for( auto it = m_renderables.begin(); it != m_renderables.end(); it++ )
	{
		renderables.push_back( *it );
	}

	// decals? children? boosters?
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
		min += m_squadBoundsMin;
		max += m_squadBoundsMax;
	}
	else
	{
		min = m_squadBoundsMin;
		max = m_squadBoundsMax;
	}
	m_localAabbMin = min - m_worldPosition;
	m_localAabbMax = max - m_worldPosition;
	min = m_localAabbMin;
	max = m_localAabbMax;
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
	EveSwarmRenderablePtr renderable;
	renderable.CreateInstance();
	renderable->SetMesh( m_mesh );
	m_renderables.Append( renderable->GetRootObject() );
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
	if( m_vehicles.size() < 1 )
	{
		return Vector3( 0, 0, 0 );
	}

	SwarmVehicle v = m_vehicles[m_targetIndex];
	m_vehicles[m_targetIndex] = m_vehicles.back();
	m_vehicles.pop_back();
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
// Description:
//   Override from EveSpaceObject2
// --------------------------------------------------------------------------------
Vector3 EveSwarm::GetObjectSpaceDamageLocatorPosition( uint32_t index ) const
{
	Vector3 position = EveShip2::GetObjectSpaceDamageLocatorPosition( index );
	if( !m_count )
	{
		return position;
	}
	Matrix localTransform = *m_renderables[m_targetIndex]->GetWorldTransform() * m_invWorldTransform;
	return *D3DXVec3TransformCoord( &position, &position, &localTransform );
}

// --------------------------------------------------------------------------------
// Description:
//   Override from EveSpaceObject2
// --------------------------------------------------------------------------------
Vector3 EveSwarm::GetObjectSpaceDamageLocatorDirection( uint32_t index ) const
{
	Vector3 direction = EveShip2::GetObjectSpaceDamageLocatorDirection( index );
	if( !m_count )
	{
		return direction;
	}
	Matrix localTransform = *m_renderables[m_targetIndex]->GetWorldTransform() * m_invWorldTransform;
	return *D3DXVec3TransformNormal( &direction, &direction, &localTransform );
}

// --------------------------------------------------------------------------------
// Description:
//   Override from EveSpaceObject2
// --------------------------------------------------------------------------------
bool EveSwarm::GetDamageLocatorPosition( Vector3* out, int index, bool inWorldSpace )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	if( !m_count )
	{
		return EveShip2::GetDamageLocatorPosition( out, index, inWorldSpace );
	}
	if( ( index < 0 ) || ( index >= int( m_persistedDamageLocators.size() ) ) )
	{
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

	*out = inWorldSpace ? GetTransformedDamageLocator( index ) : GetObjectSpaceDamageLocatorPosition( index );
	
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Calculate all forces influencing the swarm vehicle i0
// --------------------------------------------------------------------------------
Vector3 EveSwarm::CalculateForces( int i0, std::vector<SwarmVehicle>& swarmers, const Vector3& followPosition, const Vector3& centerOfMass, const Vector3& alignment, const Vector3& formationDirection, const Vector3& formationSide, float timeSeconds )
{
	Vector3 force( 0, 0, 0 ), wander, separation( 0, 0, 0 ), align, cohesion, anchor, decelerate, formation;

	wander = m_behavior.m_weightWander * Calculate_Wander( swarmers[i0], m_behavior.m_wanderDistance, m_behavior.m_wanderRadius, m_behavior.m_wanderFluctuation, timeSeconds );
	cohesion = m_behavior.m_weightCohesion * Calculate_Cohesion( swarmers[i0].position, centerOfMass );
	anchor = Calculate_Cohesion( swarmers[i0].position, followPosition );
	float anchorDistance = D3DXVec3Length( &anchor ); 
	anchorDistance = TriLinearize( m_behavior.m_anchorRadius0, m_behavior.m_anchorRadius1, anchorDistance );
	anchor = anchorDistance * m_behavior.m_weightAnchor * anchor;
	align = m_behavior.m_weightAlign * alignment;
	decelerate = swarmers[i0].velocity * -m_behavior.m_weightDecelerate;
	TriVectorClampLength( &decelerate, m_behavior.m_maxDeceleration );

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
		float rankMultiplier = floor( 0.5f * (float)i0 + 0.5f );
		formationPosition = formationPosition - formationDirection * m_behavior.m_formationDistance * rankMultiplier + formationSide * m_behavior.m_formationDistance * rankMultiplier * 0.5f;
	}
	else if( i0 )
	{
		float rankMultiplier = floor( 0.5f * (float)i0 + 0.5f );
		formationPosition = formationPosition - formationDirection * m_behavior.m_formationDistance * rankMultiplier - formationSide * m_behavior.m_formationDistance * rankMultiplier * 0.5f;
	}
	formation = m_behavior.m_weightFormation * Calculate_Cohesion( swarmers[i0].position, formationPosition );
	
	// Debug info
	if( m_debugShowForces && m_debugInfo.size() > (unsigned)i0 )
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
	Vector3 d;
	D3DXVec3Subtract( &d, &p1, &p0 );
	return d;
}

// --------------------------------------------------------------------------------
// Description:
//   Force that pushes p0 away from p1 depending on distance
// --------------------------------------------------------------------------------
inline Vector3 EveSwarm::Calculate_Separation( Vector3 p0, Vector3 p1 )
{
	Vector3 d;
	D3DXVec3Subtract( &d, &p0, &p1 );
	float length = D3DXVec3Length( &d );
	if( length == 0.f )
	{
		return Vector3( TriRand() - 0.5f, TriRand() - 0.5f, TriRand() - 0.5f );
	}
	return *D3DXVec3Normalize( &d, &d ) * m_behavior.m_separationDistance / length;
}

// --------------------------------------------------------------------------------
// Description:
//   A random wandering behavior
// --------------------------------------------------------------------------------
Vector3 EveSwarm::Calculate_Wander( SwarmVehicle& s, float wanderDistance, float radius, float fluctuation, float t )
{
	// Evolve the target point on the 'sphere' around a point wanderDistance in front of our swarmer
	Vector3 target = s.wanderTarget;
	Vector3 newOffset = Vector3( 2*TriRand() - 1.f, 2*TriRand() - 1.f, 2*TriRand() - 1.f );
	D3DXVec3Normalize( &newOffset, &newOffset );
	newOffset *= fluctuation * radius * t;
	target += newOffset;
	D3DXVec3Normalize( &target, &target );
	target *= radius;
	s.wanderTarget = target;

	// And calculate the final target force
	if( D3DXVec3LengthSq( &s.velocity ) != 0 )
	{
		D3DXVec3Normalize( &target, &s.velocity );
	}
	else
	{
		// start in the wander direction if no velocity
		D3DXVec3Normalize( &target, &s.wanderTarget );
	}
	target = target * wanderDistance + s.wanderTarget;
	return target;
}



