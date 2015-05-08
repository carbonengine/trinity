////////////////////////////////////////////////////////////
//
//    Created:   July 2011
//    Copyright: CCP 2011
//
#include "StdAfx.h"
#include "EveTurretFiringFX.h"
#include "EveUpdateContext.h"
#include "EveStretch.h"
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
	m_firingDuration( 1000.f )
{
	for( unsigned int i = 0; i < MUZZLECOUNT_MAX; ++i )
	{
		m_perMuzzleData[i].started = false;
		m_perMuzzleData[i].readyToStart = false;
		m_perMuzzleData[i].muzzlePositionBoneID = INVALID_BONE_INDEX;
		m_perMuzzleData[i].constantDelay = 0.f;
		m_perMuzzleData[i].currentStartDelay = 0.f;
		D3DXMatrixIdentity( &m_perMuzzleData[i].muzzleTransform );
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
//   Someone changed am exposed value
// --------------------------------------------------------------------------------
bool EveTurretFiringFX::OnModified( Be::Var* val )
{
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Start this firing effect with a delay!
// Arguments:
//   delay - the delay until it starts
// --------------------------------------------------------------------------------
void EveTurretFiringFX::PrepareFiring( float delay, unsigned int muzzleID )
{
	// just set a positive time delay and the effect will get triggered in ::Update()
	for( unsigned int i = 0; i < m_stretch.size(); ++i )
	{
		// launch this muzzle effect either when caller wants them all to fire (0xffffffff) or
		// an individual one is selected
		if( ( muzzleID == 0xffffffff ) || ( muzzleID == i ) )
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
		EveStretchPtr stretchEffect = m_stretch[ i ];
		for( unsigned int c = 0; c < stretchEffect->GetCurveSetCount(); ++c )
		{
			// get curveset to check for correct name
			TriCurveSetPtr curveSet = stretchEffect->GetCurveSet( c );
			float l = curveSet->GetMaxCurveDuration() / curveSet->GetTimeScale();
			if( l > maxDuration )
			{
				maxDuration = l;
			}
		}
	}
	return maxDuration;
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
	EveStretchPtr stretchEffect = m_stretch[ muzzleID ];
	for( unsigned int c = 0; c < stretchEffect->GetCurveSetCount(); ++c )
	{
		// get curveset to check for correct name
		TriCurveSetPtr curveSet = stretchEffect->GetCurveSet( c );
		if( curveSet )
		{
			if( curveSet->GetName() == "play_start" )
			{
				curveSet->PlayFrom( -m_perMuzzleData[ muzzleID ].currentStartDelay );
			}
			else if( curveSet->GetName() == "play_loop" )
			{
				curveSet->PlayFrom( -m_perMuzzleData[ muzzleID ].currentStartDelay );
			}
			else if( curveSet->GetName() == "play_end" )
			{
				curveSet->Stop();
			}
		}
	}

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
	for( unsigned int m = 0; m < m_stretch.size(); ++m )
	{
		// get the running effect
		EveStretchPtr stretchEffect = m_stretch[ m ];
		for( unsigned int c = 0; c < stretchEffect->GetCurveSetCount(); ++c )
		{
			// get curveset to check for correct name
			TriCurveSetPtr curveSet = stretchEffect->GetCurveSet( c );
			if( curveSet )
			{
				if( curveSet->GetName() == "play_start" )
				{
					curveSet->Stop();
				}
				else if( curveSet->GetName() == "play_loop" )
				{
					curveSet->Stop();
				}
				else if( curveSet->GetName() == "play_end" )
				{
					curveSet->Play();
				}
			}
		}

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
bool EveTurretFiringFX::ReadyToFire()
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
//   time - Blue total time
//   deltaT - time delta since last frame
// Return value:
//   Returns true if the effect has just fired
// --------------------------------------------------------------------------------
bool EveTurretFiringFX::Update( Be::Time time, float deltaT )
{
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
			EveStretchPtr stretchEffect = m_stretch[i];

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
					if( m_useMuzzleTransform )
					{
						// pass the whole transform to the stretch effect
						stretchEffect->SetSourceTransform( m_perMuzzleData[i].muzzleTransform );
					}
					else
					{
						// extract the point from the transform matrix
						stretchEffect->SetSourcePosition( Vector3( m_perMuzzleData[i].muzzleTransform._41, m_perMuzzleData[i].muzzleTransform._42, m_perMuzzleData[i].muzzleTransform._43 ) );
					}
					// target
					stretchEffect->SetDestinationPosition( m_endPosition );
					// toggle source and dest effects
					stretchEffect->SetDisplaySourceObject( GetDisplaySourceObject() );
					stretchEffect->SetDisplayDestObject( GetDisplayDestObject() );
					// direction
					stretchEffect->SetIsNegZForward( true );
				}

				EveUpdateContext ctx( time );
				stretchEffect->Update( ctx );
			}
			else
			{
				// ALWAYS update the stretcher, no matter if it fires ot not, there might be some
				// curveset animation going on!
				EveUpdateContext ctx( time );
				stretchEffect->UpdateCurves( ctx );
			}
		}
	}

	return retVal;
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
void EveTurretFiringFX::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables )
{
	// display?
	if( !m_display )
	{
		return;
	}

	// don't render if not firing
	if( !m_isFiring )
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
				m_stretch[i]->GetRenderables( frustum, renderables, m );
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Get the bounds for the firing effect.
// Arguments:
//   bounds - the bounding sphere for the firing effect
// Return value:
//   true if the firing effect has a valid bounding sphere, false otherwise
// --------------------------------------------------------------------------------
bool EveTurretFiringFX::GetBoundingSphere( Vector4& bounds ) const
{
	if( !m_display || !m_isFiring )
	{
		return false;
	}

	bool boundsInitialized = false;
	Vector4 v;
	Matrix m;
	for( unsigned int i = 0; i < m_stretch.size(); ++i )
	{
		if( m_perMuzzleData[i].started && ( m_firingDuration >= m_perMuzzleData[i].elapsedTime || m_isLoopFiring ) )
		{
			if( m_stretch[i]->GetBoundingSphere( v, EVE_BOUNDS_WITH_CHILDREN ) )
			{
				BoundingSphereSetOrUpdate( v, bounds, boundsInitialized );
				boundsInitialized = true;
			}
		}
	}
	return boundsInitialized;
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

