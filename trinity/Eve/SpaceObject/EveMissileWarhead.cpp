////////////////////////////////////////////////////////////
//
//    Created:   February 2012
//    Copyright: CCP 2012
//
#include "StdAfx.h"
#include "Utilities/BoundingSphere.h"

#include "EveMissileWarhead.h"
#include "Eve/EveUpdateContext.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteSet.h"
#include "Tr2Mesh.h"
#include "include/TriMath.h"
#include "TriFrustum.h"

#include "Particle/Tr2GpuSharedEmitter.h"

// keep track of missiles
CCP_STATS_DECLARE( eveVisibleWarheadObjects, "Trinity/Missiles/visibleWarheadObjects", true, CST_COUNTER_LOW, "Number of individual warheads visible in this frame.");

extern float g_eveSpaceSceneMediumDetailThreshold;
extern float g_eveSpaceSceneVisibilityThreshold;


// --------------------------------------------------------------------------------
// Description:
//   Initialize data members, set everything to inlavid/empty. And init some
//   random values we keep to make this warhead visually unique.
// --------------------------------------------------------------------------------
EveMissileWarhead::EveMissileWarhead( IRoot* lockobj ) :
	EveTransform( lockobj ),
	m_warheadLength( 1 ),
	m_warheadRadius( 1 ),
	m_state( STATE_DELAYED ),
	m_flyingTime( 0.f ),
	m_impactDuration( 0.6f ),
	m_impactSize( 0.f ),
	m_posLastFrame( 0.f, 0.f, 0.f ),
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
	m_doSpread( true ),
	m_lastPositionValid( false ),
	m_currentOffsetTransform( IdentityMatrix() ),
	m_pathOffsetNoiseScale( 0 ),
	m_pathOffsetNoiseSpeed( 1 )
{
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
//   Implements IEveSpaceObject2. Registers sprite set with quad renderer.
// --------------------------------------------------------------------------------
void EveMissileWarhead::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	if( m_spriteSet )
	{
		m_spriteSet->RegisterWithQuadRenderer( quadRenderer );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Implements IEveSpaceObject2. Adds sprites to quad renderer.
// --------------------------------------------------------------------------------
void EveMissileWarhead::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer )
{
	if( !m_startDataValid )
	{
		return;
	}

	if( m_state == STATE_DEAD )
	{
		return;
	}

	if( m_spriteSet )
	{
		m_spriteSet->AddToQuadRenderer( quadRenderer, m_worldTransform, 1.0f, 1.0f, nullptr, 0 );
	}
}

// --------------------------------------------------------------------------------
void EveMissileWarhead::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform )
{
	m_isVisible = false;

	// don't give out renderables until this warhead has valid data
	if( !m_startDataValid || m_state == STATE_DEAD )
	{
		return;
	}

	m_lodLevel = TR2_LOD_LOW;

	// is this one here enabled?
	if( ( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() ) || !m_display )
	{
		return;
	}
	
	m_isVisible = true;
	UpdateViewDependentData( frustum, parentTransform );
	
	if( m_mesh )
	{
		Vector4 boundingSphere;
		if( GetBoundingSphere( boundingSphere ) )
		{
			// check visibility with camera or, if threshold set to negative, no culling
			if( frustum.IsSphereVisible( &boundingSphere ) )
	 		{
				float estimatedSize = frustum.GetPixelSizeAccross( &boundingSphere );
				if( estimatedSize >= g_eveSpaceSceneMediumDetailThreshold )
				{
					m_lodLevel = TR2_LOD_HIGH;
				}
				// we use g_eveSpaceSceneVisibilityThreshold insdead of g_eveSpaceSceneLowDetailThreshold because we
				// completely hide warhead mesh on low LOD
				else if( estimatedSize >= g_eveSpaceSceneVisibilityThreshold )
				{
					m_lodLevel = TR2_LOD_MEDIUM;
				}
			}
		}
	}

	// update stats if we see it
	if( m_isVisible )
	{
		CCP_STATS_INC( eveVisibleWarheadObjects );
	}
}

// --------------------------------------------------------------------------------
bool EveMissileWarhead::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery ) const
{
	sphere = Vector4( 0, 0, 0.5f * m_warheadLength, 0.5f * m_warheadLength );
	BoundingSphereTransform( m_worldTransform, sphere );
	return true;
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
void EveMissileWarhead::GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors )
{
	// don't give out renderables until this warhead has valid data
	if( !m_isVisible || m_lodLevel <= TR2_LOD_LOW )
	{
		return;
	}
	
	if( m_mesh )
	{
		renderables.push_back( this );
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
	sphere = Vector4( 0, 0, 0.5f * m_warheadLength, 0.5f * m_warheadLength );
	// transform it with our surrent per-warhead matrix to get
	// the sphere into EveMissile space
	BoundingSphereTransform( m_currentOffsetTransform, sphere );
	return true;
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
			for( auto emIt = child->m_particleEmitters.begin(); emIt != child->m_particleEmitters.end(); ++emIt )
			{
				if( auto emitter = dynamic_cast<Tr2GpuSharedEmitter*>( *emIt ) )
				{
					emitter->Enable( enable );
				}
			}
		}
	}
	for( auto emIt = m_particleEmitters.begin(); emIt != m_particleEmitters.end(); ++emIt )
	{
		if( auto emitter = dynamic_cast<Tr2GpuSharedEmitter*>( *emIt ) )
		{
			emitter->Enable( enable );
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
	m_currentOffsetTransform = IdentityMatrix();
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
	m_startOrientation = RotationQuaternion( startTransform );
	// warhead data: translation offset
	m_currentStartOffset = Vector3( startTransform._41, startTransform._42, startTransform._43 );

	// start!
	m_currentOrientation = m_startOrientation;
	m_currentOffset = m_currentStartOffset;

	// now we have start data, so we are good and armed and ready to go!
	m_startDataValid = true;
	m_lastPositionValid = false;
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
	const float flight01 = TriClamp( m_flyingTime / estimatedTotalFlyingTime, 0.f, 1.f );
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
		m_targetLocator = target ? target->GetGoodDamageLocatorIndex( GetWorldPosition() ) : -1;
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
			m_targetLocator = target ? target->GetGoodDamageLocatorIndex( GetWorldPosition() ) : -1;
			evt = EVT_SWITCH_TARGET;
			m_state = STATE_TRACKING_FINAL;
		}
		break;
	case STATE_TRACKING_FINAL:
		break;
	case STATE_EXPLODED:
		m_state = STATE_DEAD;
		break;
	default:
		break;
	};

	return evt;
}

EveMissileWarhead::StateChangeEvent EveMissileWarhead::CheckImpact( float deltaT, float estimatedTotalAliveTime, ITriTargetable* target )
{
	StateChangeEvent evt = EVT_NONE;
	Vector3 targetPosition, posNow, posLast;

	if( m_state != STATE_TRACKING_FINAL || m_id < 0 )
	{
		return evt;
	}
	const float estimatedTotalFlyingTime = (estimatedTotalAliveTime + 0.1f) * m_speedModifier;
	// calc a value from 0 to 1 across the whole (estimated) flying time, (excluding eject-phase time and delay time)
	const float flight01 = TriClamp( ( m_flyingTime - deltaT ) / estimatedTotalFlyingTime, 0.f, 1.f );

	if( !target )
	{
		m_explosionPosition = GetWorldPosition();
		m_state = STATE_EXPLODED;
		return EVT_EXPLODE;
	}

	posNow = GetWorldPosition();
	posLast = posNow - m_movement;
	targetPosition = posNow;

	if( flight01 >= 1 || target->GetImpactPosition( targetPosition, m_targetLocator, posLast, posNow, m_explosionDistance ) )
	{
		m_explosionPosition = posNow;
		evt = EVT_EXPLODE;
		m_state = STATE_EXPLODED;

		Vector3 dir( targetPosition - posNow );

		// Check if we've gone past the impact position
		if( Dot( dir, m_movement ) < 0 )
		{
			m_explosionPosition = targetPosition;
		}

		if( m_impactSize > 0.f )
		{
			target->CreateImpact( m_targetLocator, -m_movement, m_impactDuration, m_impactSize );
		}
	}
	return evt;
}

// --------------------------------------------------------------------------------
// Description:
//   EveTransform update
// --------------------------------------------------------------------------------
void EveMissileWarhead::Update( EveUpdateContext& updateContext )
{
	double pos = m_flyingTime * m_pathOffsetNoiseSpeed + double( uintptr_t( this ) & 0xfff );
	m_pathOffset.x = float( PerlinNoise1D( pos, 1.1, 2.0, 3 ) ) * m_pathOffsetNoiseScale;
	m_pathOffset.y = float( PerlinNoise1D( pos + 10.1, 1.1, 2.0, 3 ) ) * m_pathOffsetNoiseScale;
	m_pathOffset.z = float( PerlinNoise1D( pos + 18.3, 1.1, 2.0, 3 ) ) * m_pathOffsetNoiseScale;

	m_posLastFrame -= updateContext.GetOriginShift();
	EveTransform::Update( updateContext );
	m_movement = GetWorldPosition() - m_posLastFrame;
	m_posLastFrame = GetWorldPosition();
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
void EveMissileWarhead::UpdateWarhead( float deltaT, float estimatedTotalAliveTime, const Vector3* currentBallVelocity, const Vector3* currentInheritedVelocity, const Matrix* invBallRotation, const Matrix& missileTransform, const Vector3& egoTranslation )
{
	// Per object data
	m_perObjectDataVs.InvalidateBufferData();
	m_perObjectDataPs.InvalidateBufferData();

	// calculate eject velocity vector
	Vector3 ejectVelocityDir( 0.f, 0.f, m_currentEjectVelocity );
	Matrix rotMatrix = RotationMatrix( m_startOrientation );
	ejectVelocityDir = TransformNormal( ejectVelocityDir, rotMatrix );
	// calculate missile-ball velocity in global space
	Vector3 globalBallVelocity = TransformNormal( *currentBallVelocity, *invBallRotation );

	// flying time only in "flying" states
	if( m_state >= STATE_START_TRACKING )
	{
		m_flyingTime += deltaT;
	}

	// from the total alive time caclulate the total flying time
	const float estimatedTotalFlyingTime = (estimatedTotalAliveTime + 0.1f) * m_speedModifier;


	// calc a value from 0 to 1 across the whole (estimated) flying time, (excluding eject-phase time and delay time)
	const float flight01 = TriClamp( m_flyingTime / estimatedTotalFlyingTime, 0.f, 1.f );
	// another value from 0 to 1 that reaches 1 faster
	const float quickFlight01 = TriClamp( 3.f * flight01, 0.f, 1.f );

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
	float t = TriClamp( ( m_flyingTime - m_finalDestinationTimer ) / ( estimatedTotalFlyingTime - m_finalDestinationTimer ), 0.f, 1.f );
	Vector3 modifiedOldOffset = m_oldEndOffset * ( 1.f - TriClamp( t * 2.f, 0.0f, 1.0f ) );
	m_currentEndOffset = Lerp( modifiedOldOffset, m_endOffset, t );

	// reduce all our influences and move the warhead closer to zero
	Quaternion id( 0.f, 0.f, 0.f, 1.f );
	m_currentOffset = Lerp( m_currentStartOffset, m_currentEndOffset, powf( flight01, 1.f + m_acceleration ) );

	// apply the inverse ball movement which holds the warhead in position during short eject-phase
	globalBallVelocity = Lerp( globalBallVelocity, Vector3( 0.f, 0.f, 0.f ), flight01 );
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

	Matrix worldTransform = missileTransform;
	Vector3 relativePosition = TransformCoord( m_currentOffset, worldTransform );
	Vector3 translation = m_lastRelativePosition - relativePosition + egoTranslation;
	m_lastRelativePosition = relativePosition;
	if( m_lastPositionValid )
	{
		if( m_startDataValid )
		{
			Quaternion orientationNow;
			const float distanceSq = LengthSq( translation );
			if( distanceSq > 0.f )
			{
				// move/facing direction is in global world-space, we need it in "ball"-space
				translation = TransformNormal( translation, *invBallRotation );
				// turn facing vector into a rotation
				TriQuaternionArcFromForward( &orientationNow, &translation );
				if( distanceSq < 1.f )
				{
					m_currentOrientation = Slerp( m_currentOrientation, orientationNow, distanceSq );
					m_currentOrientation = Normalize( m_currentOrientation );
				}
				else
				{
					m_currentOrientation = orientationNow;
				}
			}
		}
	}
	else
	{
		m_lastPositionValid = true;
	}

	// put it all together into a single transform matrix
	m_currentOffsetTransform = RotationMatrix( m_currentOrientation ) * TranslationMatrix( m_currentOffset );
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
	data->Initialize( this, &m_perObjectDataVs, &m_perObjectDataPs );

	return data;
}

uint32_t EveMissileWarhead::GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const
{
	if( shaderType == Tr2RenderContextEnum::PIXEL_SHADER )
	{
		return 0;
	}
	else
	{
		return
			64 +				// m_vsWorldMatrix
			16; 				// m_missileSize
	}
}

void EveMissileWarhead::UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* data )
{
	if( shaderType != Tr2RenderContextEnum::PIXEL_SHADER )
	{
		uint8_t* perObjectVS = static_cast<uint8_t*>( data );
		*reinterpret_cast<Matrix*>( perObjectVS ) = Transpose( m_worldTransform );
		perObjectVS += sizeof(Matrix);

		*reinterpret_cast<Vector4*>( perObjectVS ) = Vector4( m_warheadRadius, m_warheadLength, 0, 0 );
	}
}


void EveMissileWarhead::RenderDebugInfoFromParent( ITr2DebugRenderer2& renderer, Matrix transform )
{
	srand( static_cast<unsigned int>( reinterpret_cast<size_t>( this )));
	uint32_t color = 0xff000000 + rand() % 0x00ffffff;
	renderer.DrawLine( this, transform.GetTranslation(), this->GetWorldPosition(), color );
	renderer.DrawLine( this, this->GetWorldPosition(), TransformCoord( m_currentEndOffset, transform ), 0xff999999 );

	std::string message = "";
	switch( m_state )
	{
	case STATE_DELAYED:
		message = "STATE_DELAYED";
		break;
	case STATE_LAUNCH:
		message = "STATE_LAUNCH";
		break;
	case STATE_EJECTING:
		message = "STATE_EJECTING";
		break;
	case STATE_START_TRACKING:
		message = "STATE_START_TRACKING";
		break;
	case STATE_TRACKING_SPREAD:
		message = "STATE_TRACKING_SPREAD";
		break;
	case STATE_TRACKING_FINAL:
		message = "STATE_TRACKING_FINAL";
		break;
	case STATE_EXPLODED:
		message = "STATE_EXPLODED";
		break;
	default:
		message = "UNKNOWN";
		break;
	};

	renderer.DrawText( TRI_DBG_FONT_MEDIUM, this->GetWorldPosition(), color, message.c_str() );
}


// --------------------------------------------------------------------------------
// Description:
//   Copy all the matrices to HW
// --------------------------------------------------------------------------------
void EveMissileWarheadPerObjectData::SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
{
	Tr2PerObjectDataWithPersistentBuffers<EveMissileWarhead>::SetPerObjectDataToDevice( buffers, constantTypeMask, renderContext );
}

