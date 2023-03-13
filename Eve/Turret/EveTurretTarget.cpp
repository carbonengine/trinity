////////////////////////////////////////////////////////////
//
//    Created:   November 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "EveTurretTarget.h"

#include "include/TriMath.h"
#include "include/ITriTargetable.h"
#include "IWorldPosition.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveTurretTarget::EveTurretTarget( IRoot* lockobj ) :
	m_locator( -1 ),
	m_impactLength( -1.f ),
	m_impactDelay( -1.f ),
	m_impactID( -1 ),
	m_impactBehaviour( ImpactBehaviour::DAMAGE_LOCATOR ),
	m_targetPosition( 0.f, 0.f, 0.f ),
	m_trackingPosition( 0.f, 0.f, 0.f ),
	m_positionOld( 0.f, 0.f, 0.f ),
	m_positionOldInfluence( -1.f ),
	m_positionMiss( 0.f, 0.f, 0.f ),
	m_missQueue( "EveTurretTarget::m_missQueue" ),
	m_lastShotIsMiss( false ),
	m_lastShotTime( 0.0 ),
	m_laserMissBehaviour( false ),
	m_projectileMissBehaviour( false ),
	m_readyToFireEffect( false ),
	m_impactSize( 0.f ),
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

	IWorldPositionPtr newTargetPos;
	if( !object->QueryInterface( BlueInterfaceIID<IWorldPosition>(), (void**)&newTargetPos ) )
	{
		return false;
	}

	// only act on new targets
	if( !m_object.IsEqualObject( newTarget ) )
	{
		m_object = newTarget;
		// and trigger a fade
		m_positionOld = m_trackingPosition;
		m_positionOldInfluence = 1.f;

		// set the position of the object
		m_objectPos = newTargetPos;
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
void EveTurretTarget::StartFireAtLocator( int l, float delay, float length, const Vector3* source )
{
	// remember this locator
	m_locator = l;

	// randomize miss positionm
	m_randomMissDistanceOffset = TriFloatRandom01();
	float u = TriFloatRandom01(), v = TriFloatRandom01();
	float phi = u * 3.14159f * 2.f;
	float theta = acosf( 1.f - sqrtf( v ) ) * 2.f;
	TriVectorSpherical( &m_randomMissPositionOffset, phi, theta, 3.f );

	m_impactID = -1;

	// pop a miss/hit from the queue
	if( !PopShotMissed() && m_impactSize > 0.f )
	{
		// ok, we assume we hit! create an impact
		if( m_object )
		{
			m_impactLength = std::max( length, 0.f );
			m_impactDelay = delay;		

			// If we have a laser that has no buildup, we need to create an impact straigt away
			// Otherwise we have the chance of the laser ending on the hull, intead of the shield where it should be
			// https://jira.ccpgames.com/browse/EVEDEV-4947
			if( m_impactDelay == 0 )
			{
				GetImpactPosition( m_targetPosition, source );

				if( m_impactBehaviour == ImpactBehaviour::DAMAGE_LOCATOR )
				{
					Vector3 dirToSource( *source - m_targetPosition );
					m_impactID = m_object->CreateImpact( m_locator, dirToSource, m_impactLength, m_impactSize );
					m_impactDelay = -1.f;
				}
			}			
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Stop the firing
// --------------------------------------------------------------------------------
void EveTurretTarget::StopFireAtLocator()
{
	// clear out the locator
	m_locator = -1;
	// stopp anyy influence
	m_positionOldInfluence = -1.f;
	// reset the miss-system
	m_lastShotIsMiss = false;
	m_missQueue.clear();
}

// --------------------------------------------------------------------------------
// Description:
//   Gets the impact position based on some parameters 
// --------------------------------------------------------------------------------

void EveTurretTarget::GetImpactPosition( Vector3& out, const Vector3* source )
{
	if( m_object )
	{
		if( m_impactBehaviour == ImpactBehaviour::DAMAGE_LOCATOR )
		{
			m_object->GetDamageLocatorPosition( &out, m_locator, true );

			if( LengthSq(out) > 22379561604000000000000.f ) // AU squared
			{
				out = m_objectPos->GetWorldPosition();
			}
		}
		else if( m_impactBehaviour == ImpactBehaviour::CENTER )
		{
			out = m_objectPos->GetWorldPosition();
		}
		else if( m_impactBehaviour == ImpactBehaviour::SHIELD_ELLIPSOID )
		{
			if( !m_object->GetImpactPosition( out, m_locator, *source, m_objectPos->GetWorldPosition(), 0 ) )
			{
				// handle when we are inside the shield ellipsoid
				m_object->GetDamageLocatorPosition( &out, m_locator, true );
			}
		}
	}
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
		GetImpactPosition( m_targetPosition, source );

		Vector3 dirToSource( *source - m_targetPosition );

		// update the miss position
		m_object->GetMissPosition( &m_targetPosition, source, &m_positionMiss );
		m_positionMiss += m_randomMissPositionOffset;
		Vector3 direction = m_positionMiss - *source;

		if( m_laserMissBehaviour )
		{
			direction = Normalize( direction );
			m_positionMiss += direction * 250000.f;
		}
		else
		{
			float dist = Length( direction );
			direction /= dist;
			m_positionMiss += direction * ( dist + 5000.f) * ( 1.f + 0.5f * m_randomMissDistanceOffset );
		}

		if( m_impactBehaviour == ImpactBehaviour::DAMAGE_LOCATOR )
		{
			// update the impacts
			if( m_impactID != -1 )
			{
				m_object->UpdateImpact( m_targetPosition, dirToSource, m_impactID );
			}

			// what about delayed impact creation?
			if( m_impactDelay > 0.f && m_impactSize > 0.f )
			{
				m_impactDelay -= deltaT;
				if( m_impactDelay < 0.f )
				{
					m_impactID = m_object->CreateImpact( m_locator, dirToSource, m_impactLength, m_impactSize );
					m_impactDelay = -1.f;
				}
			}
		}
	}
	
	m_trackingPosition = m_targetPosition;
	// are we still fading from an old position?
	if( m_positionOldInfluence > 0.f )
	{
		// lerp the old position "in"
		m_trackingPosition = Lerp( m_targetPosition, m_positionOld, m_positionOldInfluence );
		// fadeout the influence
		m_positionOldInfluence -= deltaT;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Position that turrets track
// --------------------------------------------------------------------------------
const Vector3* EveTurretTarget::GetTrackingPosition() const
{
	if( GetShotMissed() )
	{
		return &m_positionMiss;
	}
	else
	{
		return &m_trackingPosition;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Destination position of target
// --------------------------------------------------------------------------------
const Vector3* EveTurretTarget::GetTargetPosition() const
{
	if( GetShotMissed() )
	{
		return &m_positionMiss;
	}
	else
	{
		return &m_targetPosition;
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
	if( !m_object->GetDamageLocatorPosition( position, loc, true ) )
	{
		return -1;
	}
	return loc;
}

// --------------------------------------------------------------------------------
int EveTurretTarget::FindRandomValidLocator( const Vector3& source, Vector3& position ) const
{
	if( !m_object )
	{
		return -1;
	}

	int loc = m_object->GetGoodDamageLocatorIndex( source );
	if( m_object->GetDamageLocatorPosition( &position, loc, true ) )
	{
		return loc;
	}
	loc = m_object->GetClosestDamageLocatorIndex( &source );
	if( m_object->GetDamageLocatorPosition( &position, loc, true ) )
	{
		return loc;
	}
	return -1;
}

// --------------------------------------------------------------------------------
// Description:
//   Set the internal behaviour of the hit/miss functionality
// --------------------------------------------------------------------------------
void EveTurretTarget::SetBehaviour( bool laserMiss, bool projectileMiss, float impactSize, ImpactBehaviour::Type impactBehaviour)
{
	m_laserMissBehaviour = laserMiss;
	m_projectileMissBehaviour = projectileMiss;
	m_impactSize = impactSize;
	m_impactBehaviour = impactBehaviour;
}

// --------------------------------------------------------------------------------
// Description:
//   Method to pop from the miss queue.
// --------------------------------------------------------------------------------
bool EveTurretTarget::PopShotMissed() 
{ 
	m_lastShotIsMiss = m_missQueue.empty() ? false : m_missQueue.front();
	if( !m_missQueue.empty() )
	{
		m_missQueue.pop_front(); 
	}
	return m_lastShotIsMiss;
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

// --------------------------------------------------------------------------------
// Description:
//   Pass through the GetImpactConfiguration call to the target object 
// --------------------------------------------------------------------------------
ITriTargetable::ImpactConfiguration EveTurretTarget::GetImpactConfiguration() const
{
	if( m_object )
	{
		return m_object->GetImpactConfiguration();
	}
	return ITriTargetable::ImpactConfiguration::IMPACT_INVALID;
}

// --------------------------------------------------------------------------------
// Description:
//   This determines if we show the dest-object of the stretch effect. We usually
//   do, except in some scenarios
// --------------------------------------------------------------------------------
bool EveTurretTarget::ShowDestObject() const
{
	// never show it when we are a projectile weapon and are missing
	if( m_projectileMissBehaviour && GetShotMissed() )
	{
		return false;
	}

	return true;
}
