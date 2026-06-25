// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2DistanceTracker.h"
#include "include/TriMath.h"

// --------------------------------------------------------------------------------------
// Description:
//   Default constructor.
// --------------------------------------------------------------------------------------
Tr2DistanceTracker::Tr2DistanceTracker( IRoot* lockobj ) :
	m_signedDistance( true ),
	m_distanceToClosest( true ),
	m_value( 0 ),
	m_direction( 0, 0, 0 ),
	m_sourcePosition( 0, 0, 0 ),
	m_targetPosition( 0, 0, 0 )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITriFunction interface. Updates current value.
// Arguments:
//   time - current time
// --------------------------------------------------------------------------------------
void Tr2DistanceTracker::UpdateValue( double time )
{
	if( m_source )
	{
		m_source->GetValueAt( &m_sourcePosition, time );
	}
	if( m_target )
	{
		m_target->GetValueAt( &m_targetPosition, time );
	}

	Vector3 d = m_targetPosition - m_sourcePosition;
	float projection = XMVectorGetX( XMVector3Dot( d, m_direction ) );

	if( m_distanceToClosest )
	{
		m_value = projection;
		if( !m_signedDistance )
		{
			m_value = std::abs( m_value );
		}
	}
	else
	{
		m_value = XMVectorGetX( XMVector3Length( d ) );
		if( m_signedDistance && projection < 0 )
		{
			m_value = -m_value;
		}
	}
}

bool Tr2DistanceTracker::OnModified( Be::Var* val )
{
	UpdateValue( TimeAsDouble( BeOS->GetCurrentFrameTime() ) );
	return true;
}