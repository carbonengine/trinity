////////////////////////////////////////////////////////////
//
//    Created:   January 2012
//    Copyright: CCP 2012
//
#include "StdAfx.h"
#include "Utilities/BoundingSphere.h"
#include "TriFrustum.h"

#include "EveMissile.h"
#include "EveMissileWarhead.h"
#include "Eve/EveUpdateContext.h"
#include "include/IEveReferencePoint.h"

// use options visibility threshold when to turn off turret rendering, NOT the firingeffect
extern float g_eveSpaceSceneVisibilityThreshold;

// keep track of missiles
CCP_STATS_DECLARE( eveMissileObjects, "Trinity/Missiles/missileObjects", true, CST_COUNTER_LOW, "Number of missiles (MIRVs) in this frame.");

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members, set everything to invalid/empty
// --------------------------------------------------------------------------------
EveMissile::EveMissile( IRoot* lockobj ) :
	PARENTLOCK( m_warheads ),
	m_updateWarheads( true ),
	m_inheritedStartVelocity( 0.f, 0.f, 0.f ),
	m_inheritedVelocity( 0.f, 0.f, 0.f ),
	m_time( 0.f ),
	m_estimatedTotalAliveTime( 1.f ),
	m_targetRadius( 0.f ),
	m_lastValidSpeed( 0.f )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Cleanup
// --------------------------------------------------------------------------------
EveMissile::~EveMissile()
{
}

// --------------------------------------------------------------------------------
// Description:
//   Is called right after the redfile finishes loading. Here we disable emitting
//   particles of the trails. This is because there is slight delay to the
//   actual ejection of the missile in during that timeperiod we don't want
//   so see trail particles.
// --------------------------------------------------------------------------------
bool EveMissile::Initialize()
{
	EveSpaceObject2::Initialize();

	// at this point we just finished loading the underlying redfile, now
	// make 100% sure perticle emitting on all the warheads is disabled
	for( PEveMissileWarheadVector::const_iterator it = m_warheads.begin(); it != m_warheads.end(); ++it )
	{
		(*it)->EnableParticleEmitting( false );
	}

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Since this is a spaceobject we must provide bounding sphere information.
//   This missile's bounding sphere has to be updated every frame, because
//   we hold multiple warheads which change there relative positions every
//   frame.
// Arguments:
//   sphere - return the bounding sphere data here
// Return value:
//   Returns true if bounding sphere data valid
// --------------------------------------------------------------------------------
bool EveMissile::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	sphere = Vector4( m_boundingSphereCenter.x, m_boundingSphereCenter.y, m_boundingSphereCenter.z, m_boundingSphereRadius );
	BoundingSphereTransform( m_worldTransform, sphere );
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Implements IEveSpaceObject2. Registers warheads with quad renderer.
// --------------------------------------------------------------------------------
void EveMissile::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	for( auto it = m_warheads.begin(); it != m_warheads.end(); ++it )
	{
		( *it )->RegisterWithQuadRenderer( quadRenderer );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Implements IEveSpaceObject2. Adds warhead sprites to quad renderer.
// --------------------------------------------------------------------------------
void EveMissile::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer )
{
	for( auto it = m_warheads.begin(); it != m_warheads.end(); ++it )
	{
		( *it )->AddQuadsToQuadRenderer( frustum, quadRenderer );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Main update method called every frame. Here we do all sorts of things: get
//   the position/orientation of the underlying ball and the target ball (if there
//   is any: bombs don't have targets!), update each warheads state and position
//   and calculate the bounding sphere surrounding all warheads on this missile.
// Arguments:
//   time - current game time
// --------------------------------------------------------------------------------
void EveMissile::UpdateSyncronous( EveUpdateContext& updateContext )
{
	EveSpaceObject2::UpdateSyncronous( updateContext );
	Be::Time time = updateContext.GetTime();
	float deltaT = updateContext.GetDeltaT();

	// get inv ball rotation, is needed for update of each warhead
	Matrix invBallRotationMatrix = IdentityMatrix();
	if( m_ballRotation )
	{
		Quaternion quat( 0.f, 0.f, 0.f, 1.f );
		// get rotation quaternion of our ball
		m_ballRotation->Update( &quat, time );
		// we may need the ball rotation matrix
		// inverse it to get inverse!
		quat = Inverse( quat );
		// turn it into the matrix we need
		invBallRotationMatrix = RotationMatrix( quat );
	}

	// time goes on
	m_time += deltaT;

	// get data from destiny
	Vector3 myPosition( 0.f, 0.f, 0.f );
	Vector3 myVelocity( 0.f, 0.f, 0.f );
	if( m_ballPosition && m_target )
	{
		// Estimate the time to target each frame, this INCLUDES delay and eject time
		m_ballPosition->GetValueAt( &myPosition, time );
		m_ballPosition->GetValueDotAt( &myVelocity, time );
		Vector3 targetPositionWS;
		m_target->GetDamageLocatorPosition( &targetPositionWS, -1, true );

		// calc speed
		float speed = Length( myVelocity );

		// is speed of zero is dangerous
		if( speed > 0.f )
		{
			// update the total alive time based on time already passed and distance / speed of missile ball to target
			Vector3 dir(myPosition - targetPositionWS);
			m_estimatedTotalAliveTime = m_time + ( Length( dir ) - m_targetRadius ) / speed;
			m_lastValidSpeed = speed;
		}
		else if( m_lastValidSpeed > 0.f )
		{
			// This means the ball has hit the target, but the warhead may still on the way to the target
			Vector3 dir(myPosition - targetPositionWS);
			myVelocity = Normalize( dir );
			myVelocity *= m_lastValidSpeed;
		}
	}

	Vector3 worldPos = GetWorldPosition();

	// update the submissiles aka warheads
	for( EveMissileWarheadVector::const_iterator it = m_warheads.begin(); it != m_warheads.end(); ++it )
	{
		EveMissileWarhead* wh = *it;
		// Handle the warhead state
		const EveMissileWarhead::StateChangeEvent evt = wh->UpdateState( deltaT, m_estimatedTotalAliveTime, m_target ) ;

		if( wh->GetState() != EveMissileWarhead::STATE_DEAD )
		{
			Vector3 locatorPositionWS(worldPos);
			Matrix locatorMatrix;

			if( m_target )
			{
				m_target->GetDamageLocatorPosition( &locatorPositionWS, wh->GetTargetLocator(), true );
			}
			const Vector3 locatorOffset = locatorPositionWS - worldPos;
			locatorPositionWS = TransformCoord( locatorOffset, invBallRotationMatrix );
			locatorMatrix = TranslationMatrix( locatorPositionWS );

			wh->UpdateEndTransform( locatorMatrix, evt == EveMissileWarhead::EVT_SWITCH_TARGET );

			// update warhead position, orientation, etc. if enabled
			if( m_updateWarheads )
			{
				wh->UpdateWarhead( deltaT, m_estimatedTotalAliveTime, &myVelocity, &m_inheritedVelocity, &invBallRotationMatrix, m_worldTransform, updateContext.GetOriginShift() );
			}

			// standard update method, goes straight into EveTransform::Update()
			wh->Update( updateContext );
		}

		const EveMissileWarhead::StateChangeEvent evt2 = wh->CheckImpact( deltaT, m_estimatedTotalAliveTime, m_target ) ;
		if( evt2 == EveMissileWarhead::EVT_EXPLODE )
		{
			if( m_callback )
			{
				D3DPERF_EVENT(L"EveMissile explosion callback");
				m_callback.CallVoid( wh->GetWarheadID() );
			}
		}
	}

	// update the bounding sphere: take all warheads of this MIRV into account
	RebuildMissileBoundingSphere();
}

void EveMissile::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform )
{
	EveSpaceObject2::UpdateVisibility( frustum, parentTransform );

	// collect the renderables from every warhead this MIRV has
	for( EveMissileWarheadVector::const_iterator it = m_warheads.begin(); it != m_warheads.end(); ++it )
	{
		EveMissileWarhead* warhead = (*it);
		 
		// apply the offset transform to the transform of this spaceobject
		Matrix subMissileTransform = warhead->GetCurrentOffsetTransform() * m_worldTransform;

		// final call
		warhead->UpdateVisibility( frustum, subMissileTransform );

		// Propagate warhead LODs to the missile.
		m_lodLevel = EveLODHelper::MergeLOD( m_lodLevel, warhead->GetLODLevel() );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Trinity wanst renderables, we give them out here: collect the renderables
//   from each attached warhead, but override the transform. The warhead
//   knows it transform!
// Arguments:
//   frustum - current view frustum, to do culling etc.
//   renderables - list of renderables we want to add to
//   parentTransform - can be under some EveTransform
// --------------------------------------------------------------------------------
void EveMissile::GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors )
{
	EveSpaceObject2::GetRenderables( renderables, impostors );

	// keep stats up
	CCP_STATS_INC( eveMissileObjects );
	// collect the renderables from every warhead this MIRV has
	for( EveMissileWarheadVector::const_iterator it = m_warheads.begin(); it != m_warheads.end(); ++it )
	{
		(*it)->GetRenderables( renderables, impostors );
	}
}

// --------------------------------------------------------------------------------
void EveMissile::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	EveSpaceObject2::GetDebugOptions( options );
	options.insert( "Missiles" );
	for( auto it = begin( m_warheads ); it != end( m_warheads ); ++it )
	{
		( *it )->GetDebugOptions( options );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Here we can output some debug info, bounding boxes etc.
// Arguments:
//   renderContext - current rendercontext
// --------------------------------------------------------------------------------
void EveMissile::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	EveSpaceObject2::RenderDebugInfo( renderer );

	if( renderer.HasOption( GetRawRoot(), "Missiles" ) )
	{
		renderer.DrawSphere( this, m_worldTransform, 50, 10, Tr2DebugRenderer::Wireframe, 0xff4444ff );
		for( auto it = begin( m_warheads ); it != end( m_warheads ); ++it )
		{
			( *it )->RenderDebugInfoFromParent( renderer, m_worldTransform );
		}
	}
	if( renderer.HasOption( GetRawRoot(), "Children" ) )
	{
		for( EveMissileWarheadVector::const_iterator it = m_warheads.begin(); it != m_warheads.end(); ++it )
		{
			(*it)->RenderDebugInfo( renderer );
		}
	}
}

// -----------------------------------------------------------------------------
// Description:
//   Gets the batches for the missile. At the moment, it only calls the
//   super class's function, but will get more later...
// Arguments:
//   batches - accumulator we want to add batches to
//   batchType - what kind of batch: opaque, transparent, etc.
//   perObjectData - the perobject data of this batch
// SeeAlso:
//   EveSpaceObject2::GetBatches
// -----------------------------------------------------------------------------
void EveMissile::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	EveSpaceObject2::GetBatches( batches, batchType, perObjectData, reason );
}

// --------------------------------------------------------------------------------
// Description:
//   Since this class is not holding any renderable, we don't give out any
//   perobject data. This is done in the EveWarhead's class
// --------------------------------------------------------------------------------
Tr2PerObjectData* EveMissile::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	return NULL;
}

// --------------------------------------------------------------------------------
// Description:
//   Nothing here.
// --------------------------------------------------------------------------------
void EveMissile::RebuildCachedData( BlueAsyncRes* p )
{
	EveSpaceObject2::RebuildCachedData( p );
}

// --------------------------------------------------------------------------------
// Description:
//   This function is exposed to python and called from the missile spaceobject,
//   as soon as it is created in destiny. It sets/initializes some timers, also
//   disables all particle emitting (that gets turned on later, when we are
//   in the correct warhead's state).
// Arguments:
//   shipVelocity - the firing ship's velocity vector
//   estimatedFlyingTime - a rough estimate for flying time coming from python
// --------------------------------------------------------------------------------
void EveMissile::Start( const Vector3& shipVelocity, float estimatedFlyingTime )
{
	// ship data
	m_inheritedVelocity = shipVelocity;
	m_estimatedTotalAliveTime = estimatedFlyingTime;

	// disable particle emitting here, it will get enabled in a later state of each warhead
	for( PEveMissileWarheadVector::const_iterator it = m_warheads.begin(); it != m_warheads.end(); ++it )
	{
		(*it)->EnableParticleEmitting( false );
	}

	// start!
	m_time = 0.f;
	m_inheritedStartVelocity = m_inheritedVelocity;
	m_lastValidSpeed = 0.f;
}

// --------------------------------------------------------------------------------
// Description:
//   Take all attached warheads into account and fit a bounding sphere around
//   all of them.
// Return value:
//   Returns true if bounding sphere data valid
// --------------------------------------------------------------------------------
bool EveMissile::RebuildMissileBoundingSphere()
{
	Vector4 sphere( m_boundingSphereCenter.x, m_boundingSphereCenter.y, m_boundingSphereCenter.z, m_boundingSphereRadius );
	BoundingSphereInitialize( sphere );
	for( PEveMissileWarheadVector::const_iterator it = m_warheads.begin(); it != m_warheads.end(); ++it )
	{
		Vector4 localBoundingSphere;
		if( (*it)->GetLocalBoundingSphere( localBoundingSphere ) )
		{
			// add the local bounding sphere of each warhead to the b-sphere of this missileMIRV
			BoundingSphereUpdate( localBoundingSphere, sphere );
		}
	}

	m_boundingSphereCenter.x = sphere.x;
	m_boundingSphereCenter.y = sphere.y;
	m_boundingSphereCenter.z = sphere.z;
	m_boundingSphereRadius = sphere.w;
	return true;
}
