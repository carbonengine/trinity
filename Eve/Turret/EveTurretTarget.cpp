////////////////////////////////////////////////////////////
//
//    Created:   November 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "EveTurretTarget.h"

#include "include/ITriTargetable.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveTurretTarget::EveTurretTarget( IRoot* lockobj ) :
	m_locator( -1 ),
	m_position( 0.f, 0.f, 0.f ),
	m_positionOld( 0.f, 0.f, 0.f ),
	m_positionOldInfluence( -1.f )
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
	m_object = nullptr;
	if( !object->QueryInterface( BlueInterfaceIID<ITriTargetable>(), (void**)&m_object ) )
	{
		return false;
	}

	// and trigger a fade
	m_positionOld = m_position;
	m_positionOldInfluence = 1.f;
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
	m_locator = l;
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
//   Normal timing, will update the damage position and do the target interpolation
// --------------------------------------------------------------------------------
void EveTurretTarget::Update( float deltaT )
{
	// update the position
	if( m_object )
	{
		m_object->GetDamageLocatorPosition( &m_position, m_locator );
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
	return &m_position;
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
//   Just pass through the miss function call
// --------------------------------------------------------------------------------
void EveTurretTarget::GetMissPosition( const Vector3* hit, const Vector3* source, Vector3* out )
{
	if( m_object )
	{
		m_object->GetMissPosition( hit, source, out );
	}
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

