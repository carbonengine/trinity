////////////////////////////////////////////////////////////
//
//    Created:   July 2011
//    Copyright: CCP 2011
//
#include "StdAfx.h"
#include "EveTurretFiringFX.h"
#include "Eve/EveUpdateContext.h"
#include "Eve/Renderable/Stretch/EveStretch.h"
#include "Utilities/BoundingSphere.h"

// invalids
const unsigned int INVALID_BONE_INDEX = 0xffffffff;
const unsigned int INVALID_TURRET_INDEX = 0xffffffff;

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members, set everything to inlavid/empty and call
// --------------------------------------------------------------------------------
EveTurretFiringFX::EveTurretFiringFX( IRoot* lockobj ) :
	PARENTLOCK( m_stretch ),
	m_display( true ),
	m_displaySourceObject( true ),
	m_displayDestObject( true ),
	m_endPosition( 0.f, 0.f, 0.f ),
	m_useMuzzleTransform( false ),
	m_isFiring( false ),
	m_isLoopFiring( false ),
	m_boneName( "Pos_Fire" ),
	m_firingDuration( 1000.f ),
	m_firingPeakTime( 0.f ),
	m_firingDurationOverride( -1 ),
	m_maxRadius( 3000.0 ),
	m_minRadius( 30.0 ),
	m_maxScale( 10.0 ),
	m_minScale( 1.0 ),
	m_scaleEffectTarget( false )
{
	for( unsigned int i = 0; i < MUZZLECOUNT_MAX; ++i )
	{
		m_perMuzzleData[i].started = false;
		m_perMuzzleData[i].readyToStart = false;
		m_perMuzzleData[i].muzzlePositionBoneID = INVALID_BONE_INDEX;
		m_perMuzzleData[i].constantDelay = 0.f;
		m_perMuzzleData[i].currentStartDelay = 0.f;
		m_perMuzzleData[i].muzzleTransform = IdentityMatrix();
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Cleanup
// --------------------------------------------------------------------------------
EveTurretFiringFX::~EveTurretFiringFX()
{
}

// --------------------------------------------------------------------------------
void EveTurretFiringFX::CleanUp()
{
	// shut down the firing effect and send the stop_play
	StopFiring();		
	// Kick the curves so any sound change will trigger (playing -> stop) 
	EveUpdateContext ctx( BeOS->GetCurrentFrameTime() );
	Update( ctx );
}

// --------------------------------------------------------------------------------
// Description:
//   If loading from a .red file, everything is read now
// --------------------------------------------------------------------------------
bool EveTurretFiringFX::Initialize()
{
	// check for whole curve duration
	float duration = GetCurveDuration();
	// if we get zero here, something is wrong and we are sub-optimal
	if( duration == 0.f )
	{
		CCP_LOGWARN( "EveTurretFiringFX: could not determine curve duration, will use a very number then: %s", m_name.c_str() );
		return true;
	}

	// remember it for later use
	m_firingDuration = duration;
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Use this function to tell a per-muzzle part of this effect it's source
//   location: a position determined by a bone ID
// SeeAlso:
//   TriGeometryResSkeletonData
// Arguments:
//   muzzleID - the per-muzzle ID, is between 0 and MUZZLECOUNT_MAX
//   boneID - an ID helping to identity a bone, INVALID_BONE_INDEX means "no bone"
// --------------------------------------------------------------------------------
void EveTurretFiringFX::SetMuzzleBoneID( int muzzleID, unsigned int boneID )
{
	// sanity check
	if( muzzleID >= 0 && muzzleID < MUZZLECOUNT_MAX )
	{
		m_perMuzzleData[ muzzleID ].muzzlePositionBoneID = boneID;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Lets you set each muzzle position/orientation individually
// Arguments:
//   muzzleID - the per-muzzle ID, is between 0 and MUZZLECOUNT_MAX
//   transform - pointer to 3d world transform of start
// --------------------------------------------------------------------------------
void EveTurretFiringFX::SetMuzzleTransform( int muzzleID, const Matrix* transform )
{
	// sanity check
	if( muzzleID >= 0 && muzzleID < MUZZLECOUNT_MAX )
	{
		m_perMuzzleData[ muzzleID ].muzzleTransform = *transform;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Use this function to position this effect's end in space
// Arguments:
//   endPos - pointer to 3d world position of end
// --------------------------------------------------------------------------------
void EveTurretFiringFX::SetEndPosition( const Vector3* endPos )
{
	// set
	m_endPosition = *endPos;
}

// --------------------------------------------------------------------------------
// Description:
//   Use this function to scale the destination object, using the target's radius.
// Arguments:
//   radius - the radius of the target object.
// --------------------------------------------------------------------------------
void EveTurretFiringFX::SetScaleByRadius( float radius )
{
	if ( !m_scaleEffectTarget )
	{
		return;
	}

	// The scale is a linear scale from min scale to max scale when radius is within min and max radius. Other values get clamped.
	float scale = ( radius - m_minRadius ) * ( m_maxScale - m_minScale ) / ( m_maxRadius - m_minRadius ) + m_minScale;
	scale = max( m_minScale, min( m_maxScale, scale) );

	for ( auto it = m_stretch.begin(); it != m_stretch.end(); it++ )
	{
		(*it)->SetDestObjectScale( scale );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Someone changed am exposed value
// --------------------------------------------------------------------------------
bool EveTurretFiringFX::OnModified( Be::Var* val )
{
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Called when looping when we need to reset the move object
// --------------------------------------------------------------------------------
void EveTurretFiringFX::PrepareFiringEffectMoveObjects()
{
	for( unsigned int i = 0; i < m_stretch.size(); ++i )
	{
		m_stretch[i]->StartMoving();
	}
	// we are firing
	m_isFiring = true;
}

// --------------------------------------------------------------------------------
// Description:
//   Start this firing effect with a delay!
// Arguments:
//   delay - the delay until it starts
// --------------------------------------------------------------------------------
void EveTurretFiringFX::PrepareFiring( float delay, unsigned int muzzleID, unsigned int muzzleCount )
{
	// just set a positive time delay and the effect will get triggered in ::Update()
	for( unsigned int i = 0; i < m_stretch.size(); ++i )
	{
		// launch this muzzle effect either when caller wants them all to fire (0xffffffff) or
		// an individual one is selected
		if( ( muzzleID == 0xffffffff ) || ( ( i >= muzzleID ) && ( i < muzzleID + muzzleCount ) ) )
		{
			m_perMuzzleData[i].currentStartDelay = delay + m_perMuzzleData[i].constantDelay;
			m_perMuzzleData[i].started = false;
			m_perMuzzleData[i].readyToStart = false;
			m_perMuzzleData[i].elapsedTime = 0.f;
		}
		else
		{
			// never start this muzzle!
			m_perMuzzleData[i].started = false;
			m_perMuzzleData[i].readyToStart = false;
			m_perMuzzleData[i].currentStartDelay = FLT_MAX;
			m_perMuzzleData[i].elapsedTime = 0.f;
		}
	}
	// we are firing
	m_isFiring = true;
}

// --------------------------------------------------------------------------------
// Description:
//   Calculate maximum curve duration for this effect
// --------------------------------------------------------------------------------
float EveTurretFiringFX::GetCurveDuration()
{
	float maxDuration = 0.0f;
	// check all stretch effects and see if we have to start them
	for( unsigned int i = 0; i < m_stretch.size(); ++i )
	{
		auto stretchEffect = m_stretch[ i ];
		maxDuration = std::max( maxDuration, stretchEffect->GetCurveDuration() );
	}
	return maxDuration;
}

// --------------------------------------------------------------------------------
// Description:
//   Get the start position of this firing effect. This is most likely only
//   an average, because the firing effects contains multiple beams from
//   multiple muzzles.
// --------------------------------------------------------------------------------
bool EveTurretFiringFX::GetStartPosition( Vector3& pos ) const
{
	// sum all of them up
	Vector3 p = Vector3( 0.f, 0.f, 0.f );
	uint32_t cntr = 0;
	for( size_t i = 0; i < m_stretch.size(); ++i )
	{
		if( m_perMuzzleData[ i ].started )
		{
			p += m_perMuzzleData[ i ].muzzleTransform.GetTranslation();
			++cntr;
		}
	}
	// did we get any?
	if( cntr == 0 )
	{
		return false;
	}

	// devide and return
	pos = p / (float)cntr;
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the length of this effect, based on the curve length
// --------------------------------------------------------------------------------
float EveTurretFiringFX::GetFiringDuration() const
{
	if( m_firingDurationOverride >= 0 )
	{
		return m_firingDurationOverride;
	}
	return m_firingDuration;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the time to peak of
// --------------------------------------------------------------------------------
float EveTurretFiringFX::GetFiringPeakTime() const
{
	return m_firingPeakTime;
}

// --------------------------------------------------------------------------------
const char* EveTurretFiringFX::GetFiringBoneName() const
{
	return m_boneName.c_str();
}

// --------------------------------------------------------------------------------
// Description:
//   Start a muzzle effect, usually means calling ::Play() on the right
//   curvesets. The right curveset is determined with a name.
// Arguments:
//   muzzleID - muzzle id thes effects belongs to
// --------------------------------------------------------------------------------
void EveTurretFiringFX::StartMuzzleEffect( int muzzleID )
{
	// fire!
	auto stretchEffect = m_stretch[ muzzleID ];
	stretchEffect->StartFiring( m_perMuzzleData[muzzleID].currentStartDelay );

	// set this effect to started
	m_perMuzzleData[ muzzleID ].started = true;
	m_perMuzzleData[ muzzleID ].readyToStart = false;
}

// --------------------------------------------------------------------------------
// Description:
//   Stop all muzzle effects, usually means calling ::Stop() on the right
//   curveset. The right curveset is determined with a name.
// --------------------------------------------------------------------------------
void EveTurretFiringFX::StopFiring()
{
	if( !m_isFiring )
	{
		return;
	}

	for( unsigned int m = 0; m < m_stretch.size(); ++m )
	{
		// get the running effect
		auto stretchEffect = m_stretch[ m ];
		stretchEffect->StopFiring();

		// set this effect to ended
		m_perMuzzleData[ m ].started = false;
		m_perMuzzleData[ m ].readyToStart = false;
		m_perMuzzleData[ m ].currentStartDelay = 0.f;
		m_perMuzzleData[ m ].elapsedTime = 0.f;
	}
	// no longer firing
	m_isFiring = false;
}

// --------------------------------------------------------------------------------
// Description:
//   Returns true if any muzzle is not started but is ready to start
// --------------------------------------------------------------------------------
bool EveTurretFiringFX::ReadyToFire() const
{
	for( unsigned int i = 0; i < m_stretch.size(); ++i )
	{
		if( m_perMuzzleData[i].elapsedTime < m_firingDuration || m_isLoopFiring )
		{
			if( !m_perMuzzleData[i].started && m_perMuzzleData[i].readyToStart)
			{
				return true;
			}
		}
	}
	return false;
}

// --------------------------------------------------------------------------------
// Description:
//   First thing to do iterate through all the stretch effects and see if the
//   time is right to start playing curveSets. If a stretch effect is running,
//   then update both its source and destination positions.
//   Check also if we have just started an effect and return that as boolean
//   information to the caller.
// SeeAlso:
//   EveStretch
// Arguments:
//   updateContext - scene update context
// Return value:
//   Returns true if the effect has just fired
// --------------------------------------------------------------------------------
bool EveTurretFiringFX::Update( EveUpdateContext& updateContext )
{
	float deltaT = updateContext.GetDeltaT();
	bool retVal = false;
	
	// check all stretch effects and see if we have to start them
	for( unsigned int i = 0; i < m_stretch.size(); ++i )
	{
		if( m_perMuzzleData[i].started )
		{
			m_perMuzzleData[i].elapsedTime += deltaT;
		}

		if( m_perMuzzleData[i].elapsedTime < m_firingDuration || m_isLoopFiring )
		{
			auto stretchEffect = m_stretch[i];

			// do not do all calculations if we are not firing -> waste
			if( m_isFiring )
			{
				// cannot start firing effect directly when entering FIRE state, cause they might have a delay...
				if( !m_perMuzzleData[i].started )
				{
				
					if( m_perMuzzleData[i].readyToStart )
					{
						// play two parts of the firing effect
						StartMuzzleEffect( i );
						m_perMuzzleData[i].currentStartDelay = 0.f;
						m_perMuzzleData[i].elapsedTime = 0.f;
						// don't forget to signal the firing event
						retVal = true;
					}
					else
					{
						// reduce delay time, maybe we should start?
						m_perMuzzleData[i].currentStartDelay -= deltaT;
					}

					if( m_perMuzzleData[i].currentStartDelay <= 0.f )
					{
						// start firing effect next frame
						m_perMuzzleData[i].readyToStart = true;
					}
				}

				// update the stretch effect
				if( m_perMuzzleData[i].started )
				{
					// use complete source transform or only 3d sourcepoint?
					if( m_useMuzzleTransform && ( m_perMuzzleData[i].muzzlePositionBoneID != INVALID_BONE_INDEX ) )
					{
						// pass the whole transform to the stretch effect
						stretchEffect->SetFiringTransform( m_perMuzzleData[i].muzzleTransform, m_endPosition );
					}
					else
					{
						// extract the point from the transform matrix
						stretchEffect->SetFiringTransform( m_perMuzzleData[i].muzzleTransform.GetTranslation(), m_endPosition );
					}
					// toggle source and dest effects
					stretchEffect->DisplayEndPoints( GetDisplaySourceObject(), GetDisplayDestObject() );
				}

				stretchEffect->Update( updateContext );
			}
			else
			{
				// ALWAYS update the stretcher, no matter if it fires ot not, there might be some
				// curveset animation going on!
				stretchEffect->UpdateInactive( updateContext );
			}
		}
	}

	return retVal;
}

void EveTurretFiringFX::UpdateVisibility( const TriFrustum& frustum )
{
	if( !m_display || !m_isFiring )
	{
		return;
	}

	Matrix m;
	for( unsigned int i = 0; i < m_stretch.size(); ++i )
	{
		if( m_perMuzzleData[i].started )
		{
			if ( m_firingDuration >= m_perMuzzleData[i].elapsedTime || m_isLoopFiring )
			{
				m_stretch[i]->UpdateVisibility( frustum, m );
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Standard way of rendering in Trinity. Iterate through all EveStretch effect
//   and put their renderables into list
// Arguments:
//   frustum - the current view frustum of the current frame
//   renderables - a vector for all the renderable we want to render
// SeeAlso:
//   ITr2Renderable, EveStretch
// --------------------------------------------------------------------------------
void EveTurretFiringFX::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( !m_display || !m_isFiring )
	{
		return;
	}

	for( unsigned int i = 0; i < m_stretch.size(); ++i )
	{
		if( m_perMuzzleData[i].started )
		{
			if ( m_firingDuration >= m_perMuzzleData[i].elapsedTime || m_isLoopFiring )
			{
				m_stretch[i]->GetRenderables( renderables );
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Just query the number of per-muzzle effects in this firing effect
// Return value:
//   the number of muzzle effects
// --------------------------------------------------------------------------------
unsigned int EveTurretFiringFX::GetPerMuzzleEffectCount() const
{
	return (unsigned int)m_stretch.size();
}

// --------------------------------------------------------------------------------
// Description:
//   Just query the stored bone ID for each muzzle
// Return value:
//   the previously stored bone ID
// --------------------------------------------------------------------------------
unsigned int EveTurretFiringFX::GetPerMuzzleBoneID( int muzzleID ) const
{
	// sanity check
	if( muzzleID >= 0 && muzzleID < MUZZLECOUNT_MAX )
	{
		return m_perMuzzleData[ muzzleID ].muzzlePositionBoneID;
	}
	// error
	return INVALID_TURRET_INDEX;
}

// --------------------------------------------------------------------------------
// Description:
//   Just query if this effect loops endlessly or ends itself
// Return value:
//   True if is looping endlessly
// --------------------------------------------------------------------------------
bool EveTurretFiringFX::IsLooping() const
{
	return m_isLoopFiring;
}

// --------------------------------------------------------------------------------
void EveTurretFiringFX::GetLights( Tr2LightManager& lightManager ) const
{
	if( !m_display || !m_isFiring )
	{
		return;
	}

	for( auto it = m_stretch.begin(); it != m_stretch.end(); ++it )
	{
		( *it )->GetLights( lightManager );
	}
}