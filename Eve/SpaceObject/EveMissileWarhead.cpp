////////////////////////////////////////////////////////////
//
//    Created:   February 2012
//    Copyright: CCP 2012
//
#include "StdAfx.h"
#include "Utilities/BoundingSphere.h"

#include "EveMissileWarhead.h"
#include "Tr2Mesh.h"
#include "include/TriMath.h"

// keep track of missiles
CCP_STATS_DECLARE( eveVisibleWarheadObjects, "Trinity/Missiles/visibleWarheadObjects", true, CST_COUNTER_LOW, "Number of individual warheads visible in this frame.");

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members, set everything to inlavid/empty. And init some
//   random values we keep to make this warhead visually unique.
// --------------------------------------------------------------------------------
EveMissileWarhead::EveMissileWarhead( IRoot* lockobj ) :
	EveTransform( lockobj ),
	m_state( STATE_DELAYED ),
	m_flyingTime( 0.f ),
	m_currentStartOffset( 0.f, 0.f, 0.f ),
	m_startOrientation( 0.f, 0.f, 0.f, 1.f ),
	m_startDataValid( false ),
	m_oldEndOffset( 0.f, 0.f, 0.f ),
	m_currentEndOffset( 0.f, 0.f, 0.f ),
	m_endOffset( 0.f, 0.f, 0.f ),
	m_currentOffset( 0.f, 0.f, 0.f ),
	m_currentOrientation( 0.f, 0.f, 0.f, 1.f ),
	m_currentEjectVelocity( 0.f ),
	m_currentDurationEjectPhase( 0.f ),
	m_finalDestinationTimer( 0.f ),
	m_finalTargetTime( 0.f ),
	m_pathOffset( 0.f, 0.f, 0.f ),
	m_durationEjectPhase( 0.f ),
	m_startEjectVelocity( 0.f ),
	m_acceleration( 1.f ),
	m_targetLocator( -1 ),
	m_id( -1 ),
	m_speedModifier( 1.0f ),
	m_explosionDistance( 0.0f ),
	m_maxExplosionDistance( 40.0f ),
	m_explosionPosition( 0.f, 0.f, 0.f ),
	m_bombFlightpath( false ),
	m_doSpread( true )
{
	D3DXMatrixIdentity( &m_currentOffsetTransform );
	m_speedModifier = 1.04f - TriRand() * 0.08f;
	m_finalTargetTime = 0.75f - TriRand() * 0.1f;
}

// --------------------------------------------------------------------------------
// Description:
//   Cleanup
// --------------------------------------------------------------------------------
EveMissileWarhead::~EveMissileWarhead()
{
}

// --------------------------------------------------------------------------------
// Description:
//   Trinity wanst renderables, we give them out here: collect the renderables
//   from this warhead. Don't render anything if start data is not set from
//   python yet or we are in the DEAD state (means: exploded)
// Arguments:
//   frustum - current view frustum, to do culling etc.
//   renderables - list of renderables we want to add to
//   parentTransform - we are under EveMissile and thats out position
// --------------------------------------------------------------------------------
void EveMissileWarhead::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform )
{
	// don't give out renderables until this warhead has valid data
	if( !m_startDataValid )
	{
		return;
	}

	if( m_state == STATE_DEAD )
	{
		return;
	}

	// all that this warhead is, is managed by the EveTransform
	EveTransform::GetRenderables( frustum, renderables, parentTransform );

	// update stats if we see it
	if( m_isVisible )
	{
		CCP_STATS_INC( eveVisibleWarheadObjects );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Hand out the relative offset transform of the warhead.
// Return value:
//   Returns a matrix transform holding this warhead's offset
// --------------------------------------------------------------------------------
const Matrix& EveMissileWarhead::GetCurrentOffsetTransform() const
{
	return m_currentOffsetTransform;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the bounding sphere of the missile's geometry. Locally, not in
//   worldspace.
// Arguments:
//   sphere - out the bounding sphere info here
// Return value:
//   Returns true if bounding shpere data is valid
// --------------------------------------------------------------------------------
bool EveMissileWarhead::GetLocalBoundingSphere( Vector4& sphere ) const
{
	// must have mesh, is source of sphere in object-space
	if( m_mesh )
	{
		if( m_mesh->GetBoundingSphere( sphere ) )
		{
			// transform it with our surrent per-warhead matrix to get
			// the sphere into EveMissile space
			BoundingSphereTransform( m_currentOffsetTransform, sphere );
			return true;
		}
	}
	return false;
}

// --------------------------------------------------------------------------------
// Description:
//   Enables or disables all attached particle systems. This is used to enable/
//   dsiable general trails.
// Arguments:
//   enable - on or off
// --------------------------------------------------------------------------------
void EveMissileWarhead::EnableParticleEmitting( bool enable )
{
	// try to find all attached GPU particle systems
	for( PIEveTransformVector::const_iterator it = m_children.begin(); it != m_children.end(); ++it )
	{
		EveTransformPtr child;
		if( (*it)->QueryInterface( BlueInterfaceIID<EveTransform>(), (void**)&child, BEQI_SILENT ) )
		{
			// do we have an attached GPU emitter?
			for( Tr2GPUParticleEmitterVector::const_iterator emIt = child->m_particleEmittersGPU.begin(); emIt != child->m_particleEmittersGPU.end(); ++emIt )
			{
				(*emIt)->Enable( enable );
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Called to prepare this warhead's launch: right when the destiny ball is
//   created, we call this to set/init some values. This will not activate the
//   warhead, meaning not turn on rendering or turn on the trail's particles.
//   Also use this place to re-init some of the member vars, as this warhead
//   could be re-used by the recycler
// --------------------------------------------------------------------------------
void EveMissileWarhead::PrepareLaunch()
{
	// start some timers
	m_currentEjectVelocity = m_startEjectVelocity;
	m_currentDurationEjectPhase = m_durationEjectPhase;

	// distance to the ship for explosion point
	m_explosionDistance = m_maxExplosionDistance - TriRand() * m_maxExplosionDistance / 2.0f;
	m_explosionDistance *= m_explosionDistance;

	// defaults
	m_state = STATE_DELAYED;
	m_flyingTime = 0.f;
	m_currentStartOffset = Vector3( 0.f, 0.f, 0.f );
	m_startOrientation = Quaternion( 0.f, 0.f, 0.f, 1.f );
	m_startDataValid = false;
	m_oldEndOffset = Vector3( 0.f, 0.f, 0.f );
	m_currentEndOffset = Vector3( 0.f, 0.f, 0.f );
	m_endOffset = Vector3( 0.f, 0.f, 0.f );
	m_currentOffset = Vector3( 0.f, 0.f, 0.f );
	m_currentOrientation = Quaternion( 0.f, 0.f, 0.f, 1.f );
	m_finalDestinationTimer = 0.f;
	m_finalTargetTime = 0.f;
	m_targetLocator = -1;
	m_explosionPosition = Vector3( 0.f, 0.f, 0.f );

	// inits from the constructor of this class
	D3DXMatrixIdentity( &m_currentOffsetTransform );
	m_speedModifier = 1.04f - TriRand() * 0.08f;
	m_finalTargetTime = 0.75f - TriRand() * 0.1f;
}

// --------------------------------------------------------------------------------
// Description:
//   Called to launch the warhead: after some random delay, python calls this
//   to turn on rendering and trail's particles and to start flying. The missile
//   starts flying at the provided position. (hopefully including the turret's
//   position it came out of)
// Arguments:
//   startTransform - the starttransform in worldspace of the missile.
// --------------------------------------------------------------------------------
void EveMissileWarhead::Launch( const Matrix& startTransform )
{
	// warhead data: which way is the warhead pointing?
	D3DXQuaternionRotationMatrix( &m_startOrientation, &startTransform );
	// warhead data: translation offset
	m_currentStartOffset = Vector3( startTransform._41, startTransform._42, startTransform._43 );

	// start!
	m_currentOrientation = m_startOrientation;
	m_currentOffset = m_currentStartOffset;

	// now we have start data, so we are good and armed and ready to go!
	m_startDataValid = true;
}

// --------------------------------------------------------------------------------
// Description:
//   Called to update this warhead's destination, with a possible indication
//   that a new target (locator) is chosen.
// Arguments:
//   endTransform - the endtransform of the final destination of this warhead
//   switchLocators - if true the target has changed and the warhead can react ot it
// --------------------------------------------------------------------------------
void EveMissileWarhead::UpdateEndTransform( const Matrix& endTransform, bool switchLocators )
{
	// warhead data: translation offset is all we want for now, no rotation
	m_endOffset = Vector3( endTransform._41, endTransform._42, endTransform._43 );

	if( switchLocators )
	{
		// start timing of the final part of this flight by remebering when it started!
		m_finalDestinationTimer = m_flyingTime;

		// remember offset so we can nicely lerp to new offset
		m_oldEndOffset = m_currentEndOffset;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Handles the warhead's state managment. Makes sure we are in the corrct state
//   by checking certain conditions like timings or distances etc.
// Arguments:
//   deltaT - time since last frame in seconds
//   estimatedTotalAliveTime - up-to-date estimate of the total alive time
//   target - what are we hitting
// Return value:
//   Returns an enum to trigger explosions etc. due to state changes
// --------------------------------------------------------------------------------
EveMissileWarhead::StateChangeEvent EveMissileWarhead::UpdateState( float deltaT, float estimatedTotalAliveTime, ITriTargetable* target )
{
	// NB: target may be null (bombs)
	m_bombFlightpath = !target;

	StateChangeEvent evt = EVT_NONE;
	Vector3 position;
	const float estimatedTotalFlyingTime = (estimatedTotalAliveTime + 0.1f) * m_speedModifier;

	// calc a value from 0 to 1 across the whole (estimated) flying time, (excluding eject-phase time and delay time)
	const float flight01 = Clamp( m_flyingTime / estimatedTotalFlyingTime, 0.f, 1.f );
	switch( m_state )
	{
	case STATE_DELAYED:
		// wait for valid start data then we are ready to launch
		if( m_startDataValid )
		{
			m_state = STATE_LAUNCH;
		}
		break;
	case STATE_LAUNCH:
		// turn on particle emitting
		EnableParticleEmitting( true );
		m_state = STATE_EJECTING;
		break;
	case STATE_EJECTING:
		m_currentDurationEjectPhase -= deltaT;
		if( m_currentDurationEjectPhase <= 0.f )
		{
			m_currentDurationEjectPhase = 0.f;
			m_state = STATE_START_TRACKING;
		}
		break;
	case STATE_START_TRACKING:
		// get target locator from target
		m_targetLocator = target ? target->GetGoodDamageLocatorIndex( *GetWorldPosition() ) : -1;
		if( estimatedTotalAliveTime >= 5.f && m_doSpread )
		{
			m_state = STATE_TRACKING_SPREAD;
		}
		else
		{
			m_state = STATE_TRACKING_FINAL;
		}
		break;
	case STATE_TRACKING_SPREAD:
		if( flight01 >= m_finalTargetTime )
		{
			m_targetLocator = target ? target->GetGoodDamageLocatorIndex( *GetWorldPosition() ) : -1;
			evt = EVT_SWITCH_TARGET;
			m_state = STATE_TRACKING_FINAL;
		}
		break;
	case STATE_TRACKING_FINAL:
		{
			if( !target )
			{
				position = *GetWorldPosition();
			}
			else
			{
				target->GetDamageLocatorPosition( &position, m_targetLocator );
			}

			Vector3 dir(position - *GetWorldPosition());
			if( D3DXVec3LengthSq( &dir ) <= m_explosionDistance || flight01 >= 1.0f )
			{
				if( m_id > -1 )
				{
					m_explosionPosition = *GetWorldPosition();
					evt = EVT_EXPLODE;
					m_state = STATE_EXPLODED;
				}
			}
		}
		break;
	case STATE_EXPLODED:
		m_state = STATE_DEAD;
		break;
	default:
		break;
	};

	return evt;
}

// --------------------------------------------------------------------------------
// Description:
//   Updates the position/orientation of this warhead. Here is were all the
//   magic happens. Mostly a lot of lerps from the start position to the end
//   position. These calculation give the warhead it's semi-smart flying path.
// Arguments:
//   deltaT - time since last frame in seconds
//   estimatedTotalAliveTime - up-to-date estimate of the total alive time
//   currentBallVelocity - destiny ball's velocity, can be used to counter it
//   currentInheritedVelocity - launching ship's velocity at the time of launch
//   invBallRotation - inverse ball rotation to transform the ball velocity
// --------------------------------------------------------------------------------
void EveMissileWarhead::UpdateWarhead( float deltaT, float estimatedTotalAliveTime, const Vector3* currentBallVelocity, const Vector3* currentInheritedVelocity, const Matrix* invBallRotation )
{
	// calculate eject velocity vector
	Vector3 ejectVelocityDir( 0.f, 0.f, m_currentEjectVelocity );
	Matrix rotMatrix;
	D3DXMatrixRotationQuaternion( &rotMatrix, &m_startOrientation );
	D3DXVec3TransformNormal( &ejectVelocityDir, &ejectVelocityDir, &rotMatrix );
	// calculate missile-ball velocity in global space
	Vector3 globalBallVelocity;
	D3DXVec3TransformNormal( &globalBallVelocity, currentBallVelocity, invBallRotation );

	// flying time only in "flying" states
	if( m_state >= STATE_START_TRACKING )
	{
		m_flyingTime += deltaT;
	}

	// from the total alive time caclulate the total flying time
	const float estimatedTotalFlyingTime = (estimatedTotalAliveTime + 0.1f) * m_speedModifier;


	// calc a value from 0 to 1 across the whole (estimated) flying time, (excluding eject-phase time and delay time)
	const float flight01 = Clamp( m_flyingTime / estimatedTotalFlyingTime, 0.f, 1.f );
	// another value from 0 to 1 that reaches 1 faster
	const float quickFlight01 = Clamp( 3.f * flight01, 0.f, 1.f );

	// apply the eject velocity to the "virtual" start position
	if( m_state >= STATE_EJECTING )
	{
		m_currentStartOffset += ejectVelocityDir * deltaT;
	}

	// apply the inherited velocity to the "virtual" start position
	m_currentStartOffset += *currentInheritedVelocity * deltaT;

	// is the final destination data valid?
	Vector3 endOffset( 0.f, 0.f, 0.f );
	// find current bias toward the target locator
	float t = Clamp( ( m_flyingTime - m_finalDestinationTimer ) / ( estimatedTotalFlyingTime - m_finalDestinationTimer ), 0.f, 1.f );
	Vector3 modifiedOldOffset = m_oldEndOffset * ( 1.f - Clamp( t * 2.f, 0.0f, 1.0f ) );
	D3DXVec3Lerp( &m_currentEndOffset, &modifiedOldOffset, &m_endOffset, t );

	// reduce all our influences and move the warhead closer to zero
	Quaternion id( 0.f, 0.f, 0.f, 1.f );
	D3DXQuaternionSlerp( &m_currentOrientation, &m_startOrientation, &id, flight01 );
	D3DXQuaternionNormalize( &m_currentOrientation, &m_currentOrientation );
	D3DXVec3Lerp( &m_currentOffset, &m_currentStartOffset, &m_currentEndOffset, powf( flight01, 1.f + m_acceleration ) );

	// apply the inverse ball movement which holds the warhead in position during short eject-phase
	static const Vector3 zero( 0.f, 0.f, 0.f );
	D3DXVec3Lerp( &globalBallVelocity, &globalBallVelocity, &zero, flight01 );
	m_currentStartOffset -= globalBallVelocity * deltaT;

	// reduce the eject velocity, also quickly
	m_currentEjectVelocity = Lerp( m_startEjectVelocity, 0.f, powf( quickFlight01, 1.f + m_acceleration ) );

	// apply the animated offset, but be carefull: scale it down in beginning and end!
	m_currentOffset += powf( sinf( XM_PI * flight01 ), 2.f ) * m_pathOffset;
	
	// override some behaviour for bombs
	if( m_bombFlightpath )
	{
		const float bombTrackBall = pow( 1.f - quickFlight01, 2 );
		m_currentOffset *= bombTrackBall;
	}

	// find a way better warhead orientation, that exactly fits the flying path in worldspace
	// simplify: the attached gpu global particlesystem does know this facing direction, so get that!
	for( PIEveTransformVector::const_iterator it = m_children.begin(); it != m_children.end(); ++it )
	{
		EveTransformPtr child;
		if( (*it)->QueryInterface( BlueInterfaceIID<EveTransform>(), (void**)&child, BEQI_SILENT ) )
		{
			// do we have an attached GPU system?
			if( !child->m_particleEmittersGPU.empty() )
			{
				Tr2GPUParticleEmitterPtr trailEmitter = child->m_particleEmittersGPU[0];
				// does it know what we need?
				if( trailEmitter->IsLastTranslationValid() )
				{
					Vector3 moveDirection = -1.f * trailEmitter->GetLastTranslation();
					// length must be at least something otherwise it's not worth using it
					if( D3DXVec3LengthSq( &moveDirection ) > 1.f )
					{
						// move/facing direction is in global world-space, we need it in "ball"-space
						D3DXVec3TransformNormal( &moveDirection, &moveDirection, invBallRotation );
						// turn facing vector into a rotation
						TriQuaternionArcFromForward( &m_currentOrientation, &moveDirection );
					}
					else
					{
						// backup
						m_currentOrientation = m_startOrientation;
					}
				}
			}
		}
	}

	// put it all together into a single transform matrix
	D3DXMatrixAffineTransformation( &m_currentOffsetTransform, 1.f, NULL, &m_currentOrientation, &m_currentOffset );
}

// --------------------------------------------------------------------------------
// Description:
//   Provide perobject data. Is very simple just a world transform
// Arguments:
//   accumulator - used to allocate new perobject data
// Return value:
//   Returns the populated perobject data
// --------------------------------------------------------------------------------
Tr2PerObjectData* EveMissileWarhead::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	EveMissileWarheadPerObjectData* data = accumulator->Allocate<EveMissileWarheadPerObjectData>();
	if( !data )
	{
		return NULL;
	}

	// column_major for shaders
	D3DXMatrixTranspose( &data->m_world, &m_worldTransform );

	return data;
}

