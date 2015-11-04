////////////////////////////////////////////////////////////
//
//    Created:   November 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "EveTurretTarget.h"

#include "include/TriMath.h"
#include "include/ITriTargetable.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveTurretTarget::EveTurretTarget( IRoot* lockobj ) :
	m_locator( -1 ),
	m_shieldImpactID( -1 ),
	m_position( 0.f, 0.f, 0.f ),
	m_positionOld( 0.f, 0.f, 0.f ),
	m_positionOldInfluence( -1.f ),
	m_dirToSource( 0.f, 0.f, 0.f ),
	m_targetPositionMiss( 0.f, 0.f, 0.f ),
	m_missQueue( "EveTurretTarget::m_missQueue" ),
	m_lastShotIsMiss( false ),
	m_lastShotTime( 0.0 ),
	m_laserMissBehaviour( false ),
	m_projectileMissBehaviour( false ),
	m_readyToFireEffect( false ),
	m_trackMissPoint( false ),
	m_randomMissDistanceOffset( 0.5f ),
	m_randomMissPositionOffset( 0.f, 0.f, 0.f )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Byebye
// --------------------------------------------------------------------------------
EveTurretTarget::~EveTurretTarget()
{
}

// --------------------------------------------------------------------------------
// Description:
//   Just return the target object
// --------------------------------------------------------------------------------
ITriTargetablePtr EveTurretTarget::GetTargetable() const
{
	return m_object;
}

// --------------------------------------------------------------------------------
// Description:
//   Set the target object. Since this most liekly has changed, trigger
//   the targetposition smoothing (interpolating)
// --------------------------------------------------------------------------------
bool EveTurretTarget::SetTargetable( IRoot* object )
{
	// set new target object
	ITriTargetablePtr newTarget;
	if( !object->QueryInterface( BlueInterfaceIID<ITriTargetable>(), (void**)&newTarget ) )
	{
		return false;
	}

	// only act on new targets
	if( !m_object.IsEqualObject( newTarget ) )
	{
		m_object = newTarget;
		// and trigger a fade
		m_positionOld = m_position;
		m_positionOldInfluence = 1.f;
	}
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Get the locator ID
// --------------------------------------------------------------------------------
int EveTurretTarget::GetLocator() const
{
	return m_locator;
}

// --------------------------------------------------------------------------------
// Description:
//   Start the firing procedure at a given locator
// --------------------------------------------------------------------------------
void EveTurretTarget::StartFireAtLocator( int l )
{
	// remember this locator
	m_locator = l;

	// randomize miss positionm
	m_randomMissDistanceOffset = TriFloatRandom01();
	float u = TriFloatRandom01(), v = TriFloatRandom01();
	float phi = u * 3.14159f * 2.f;
	float theta = acosf( 1.f - sqrtf( v ) ) * 2.f;
	TriVectorSpherical( &m_randomMissPositionOffset, phi, theta, 3.f );

	if( m_object )
	{
//		m_shieldImpactID = m_object->CreateShieldImpact( m_locator, m_dirToSource, 5.f );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Stop the firing
// --------------------------------------------------------------------------------
void EveTurretTarget::StopFireAtLocator()
{
	// clear out the locator and also stopp anyt influence
	m_locator = -1;
	m_positionOldInfluence = -1.f;
}

// --------------------------------------------------------------------------------
// Description:
//   Normal timing, will update the damage position incl target interpolation
//   and keep a diretion vector to the source of the shooting
// --------------------------------------------------------------------------------
void EveTurretTarget::Update( float deltaT, const Vector3* source )
{
	if( m_object )
	{
		// update the position & diretion
		m_object->GetDamageLocatorPosition( &m_position, m_locator );
		m_dirToSource = *source - m_position;

		// update the impacts
		if( m_shieldImpactID != -1 )
		{
			m_object->UpdateShieldImpact( m_position, m_dirToSource, m_shieldImpactID );
		}
	}

	// are we still fading from an old position?
	if( m_positionOldInfluence > 0.f )
	{
		// lerp the old position "in"
		D3DXVec3Lerp( &m_position, &m_position, &m_positionOld, m_positionOldInfluence );
		// fadeout the influence
		m_positionOldInfluence -= deltaT;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   This is where we give out the position
// --------------------------------------------------------------------------------
const Vector3* EveTurretTarget::GetTargetPosition() const
{
	if( GetShotMissed() )
	{
		return &m_targetPositionMiss;
	}
	else
	{
		return &m_position;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Find the best locator ID and it's position for a given source (aka gun) position
// --------------------------------------------------------------------------------
int EveTurretTarget::FindClosestLocator( const Vector3* source, Vector3* position ) const
{
	// do we have a target object?
	if( !m_object )
	{
		return -1;
	}

	// find closest locator to source
	int loc = m_object->GetClosestDamageLocatorIndex( source );
	if( !m_object->GetDamageLocatorPosition( position, loc ) )
	{
		return -1;
	}
	return loc;
}

// --------------------------------------------------------------------------------
// Description:
//   Set the internal behaviour of the hit/miss functionality
// --------------------------------------------------------------------------------
void EveTurretTarget::SetBehaviour( bool laserMiss, bool projectileMiss )
{
	m_laserMissBehaviour = laserMiss;
	m_projectileMissBehaviour = projectileMiss;
}

// --------------------------------------------------------------------------------
// Description:
//   Clears the miss queue. Do this when changing target, disabling turrets etc.
// --------------------------------------------------------------------------------
void EveTurretTarget::ResetMissQueue()
{
	m_lastShotIsMiss = false;
	m_trackMissPoint = false;
	while( !m_missQueue.empty() ) 
	{
		m_missQueue.pop_front();
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Method to pop from the miss queue.
// --------------------------------------------------------------------------------
void EveTurretTarget::PopShotMissed() 
{ 
	m_lastShotIsMiss = m_missQueue.empty() ? false : m_missQueue.front();
	if( !m_missQueue.empty() )
	{
		m_missQueue.pop_front(); 
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Get the hit/miss status of the last shot info.
// --------------------------------------------------------------------------------
bool EveTurretTarget::GetShotMissed() const
{ 
	return m_lastShotIsMiss;
}

// --------------------------------------------------------------------------------
// Description:
//   Add a hit/miss to the shot queue.
// --------------------------------------------------------------------------------
void EveTurretTarget::SetShotMissed( bool missed ) 
{ 
	if( !m_trackMissPoint )
	{
		m_lastShotIsMiss = missed;
	}
	m_trackMissPoint = true;
	m_missQueue.push_back( missed );
	m_lastShotTime = TimeAsDouble( BeOS->GetActualTime() );
	// in case we get way behind, start dropping miss events, rather than infinitely accumulating.
	// should still be representative.
	while( m_missQueue.size() > 4 )
	{
		m_missQueue.pop_front();
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Still to come
// --------------------------------------------------------------------------------
void EveTurretTarget::UpdateMissPosition( const Matrix *parentMatrix )
{
	// early out if we're not tracking miss positions
	if( !m_trackMissPoint || !m_object )
	{
		m_targetPositionMiss = *GetTargetPosition();
		return;
	}

	Vector3 muzzlePos = Vector3( parentMatrix->_41, parentMatrix->_42, parentMatrix->_43 );

	m_object->GetMissPosition( &m_position, &muzzlePos, &m_targetPositionMiss );

	m_targetPositionMiss += m_randomMissPositionOffset;
	Vector3 direction = m_targetPositionMiss - muzzlePos;

	if( m_laserMissBehaviour )
	{
		D3DXVec3Normalize( &direction, &direction );
		m_targetPositionMiss += direction * 250000.f;
	}
	else
	{
		float dist = D3DXVec3Length( &direction );
		direction /= dist;
		m_targetPositionMiss += direction * ( dist + 5000.f) * ( 1.f + 0.5f * m_randomMissDistanceOffset );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Return the time of the last shot.
// --------------------------------------------------------------------------------
double EveTurretTarget::GetLastShotTime() const
{
	return m_lastShotTime;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the size of the miss queue
// --------------------------------------------------------------------------------
size_t EveTurretTarget::MissQueueSize() const
{
	return m_missQueue.size();
}

// --------------------------------------------------------------------------------
// Description:
//   Just pass through the getradius function call
// --------------------------------------------------------------------------------
float EveTurretTarget::GetRadius() const
{
	if( m_object )
	{
		return m_object->GetRadius();
	}
	return -1.f;
}

