////////////////////////////////////////////////////////////
//
//    Created:   October 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "EveRemotePositionCurve.h"

#include "include/TriMath.h"


EveRemotePositionCurve::EveRemotePositionCurve(IRoot* lockobj) :
	m_value( 0.f, 0.f, 0.f ),
	m_offsetDir1( 0.f, 0.f, 0.f ),
	m_offsetDir2( 0.f, 0.f, 0.f ),
	m_startTime( 0 ),
	m_delayTime( 0.f ),
	m_sweepTime( 1.f ),
	m_cycle( false )
{	
}

EveRemotePositionCurve::~EveRemotePositionCurve()
{
}

// --------------------------------------------------------------------------------
// Description:
//   Value getter for the ITriVectorFunction
// --------------------------------------------------------------------------------
Vector3* EveRemotePositionCurve::Update( Vector3* in, Be::Time t )
{
	// not do anything if no start point
	if( !m_startPositionCurve )
	{
		*in = Vector3( 0.f, 0.f, 0.f );
		return in;
	}

	// timing
	if( m_startTime == 0 )
	{
		m_startTime = t;
	}
	float timeSinceStart = TimeAsFloat( t - m_startTime );

	float s = 0.f;

	// delay sweep if set!
	if( timeSinceStart > m_delayTime )
	{
		// loop or not to loop!
		if( m_cycle )
		{
			s = TriClamp( fmod( ( timeSinceStart - m_delayTime ), m_sweepTime ) / m_sweepTime, 0.f, 1.f );
		}
		else
		{
			s = TriClamp( ( timeSinceStart - m_delayTime ) / m_sweepTime, 0.f, 1.f );
		}
	}

	// interpolate current offset
	Vector3 currentOffsetDir = Lerp( m_offsetDir1, m_offsetDir2, s );

	// add to start position
	Vector3 startPos;
	m_startPositionCurve->GetValueAt( &startPos, t );
	*in = m_value = startPos + currentOffsetDir;

	return in;
}

// --------------------------------------------------------------------------------
// Description:
//   Value getter for the ITriVectorFunction
// --------------------------------------------------------------------------------
Vector3* EveRemotePositionCurve::Update( Vector3* in, double time )
{
	return in;
}

// --------------------------------------------------------------------------------
// Description:
//   Value getter for the ITriVectorFunction
// --------------------------------------------------------------------------------
Vector3* EveRemotePositionCurve::GetValueAt( Vector3* in, Be::Time time )
{
	*in = m_value;
	return in;
}

// --------------------------------------------------------------------------------
// Description:
//   Value getter for the ITriVectorFunction
// --------------------------------------------------------------------------------
Vector3* EveRemotePositionCurve::GetValueAt( Vector3* in, double time )
{
	*in = m_value;
	return in;
}

// --------------------------------------------------------------------------------
// Description:
//   Value getter for the ITriVectorFunction
// --------------------------------------------------------------------------------
Vector3* EveRemotePositionCurve::GetValueDoubleDotAt( Vector3* in, Be::Time time )
{
	return in;
}

// --------------------------------------------------------------------------------
// Description:
//   Value getter for the ITriVectorFunction
// --------------------------------------------------------------------------------
Vector3* EveRemotePositionCurve::GetValueDoubleDotAt( Vector3* in, double time )
{
	return in;
}

// --------------------------------------------------------------------------------
// Description:
//   Value getter for the ITriVectorFunction
// --------------------------------------------------------------------------------
Vector3* EveRemotePositionCurve::GetValueDotAt( Vector3* in, Be::Time time )
{
	return in;
}

// --------------------------------------------------------------------------------
// Description:
//   Value getter for the ITriVectorFunction
// --------------------------------------------------------------------------------
Vector3* EveRemotePositionCurve::GetValueDotAt( Vector3* in, double time )
{
	return in;
}

// --------------------------------------------------------------------------------
// Description:
//   Value getter for the ITriVectorFunction
// --------------------------------------------------------------------------------
Vector3d* EveRemotePositionCurve::InterpolatedPosition( Vector3d* out, Be::Time time )
{
	*out = Vector3d( double(m_value.x), double(m_value.y), double(m_value.z) );
	return out;
}

