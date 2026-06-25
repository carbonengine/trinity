// Copyright © 2017 CCP ehf.

#include "StdAfx.h"
#include "Tr2RotationAdapter.h"
#include "Utilities/Vector3d.h"


// --------------------------------------------------------------------------------
Tr2RotationAdapter::Tr2RotationAdapter( IRoot* ) :
	m_value( 0, 0, 0, 1 ),
	m_currentValue( 0, 0, 0, 1 ),
	m_start( 0 ),
	m_timeScale( 1.f ),
	m_offset( 0 )
{
}

// --------------------------------------------------------------------------------
void Tr2RotationAdapter::UpdateValue( double time )
{
	if( m_curve )
	{
		m_curve->Update( &m_currentValue, GetLocalTime( time ) );
	}
}

// --------------------------------------------------------------------------------
Quaternion* Tr2RotationAdapter::Update( Quaternion* in, Be::Time time )
{
	if( m_start == 0 )
	{
		m_start = time;
	}
	if( m_curve )
	{
		m_curve->Update( &m_currentValue, GetLocalTime( time ) );
	}
	else
	{
		m_currentValue = m_value;
	}
	*in = m_currentValue;
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2RotationAdapter::Update( Quaternion* in, double time )
{
	if( m_curve )
	{
		m_curve->Update( &m_currentValue, GetLocalTime( time ) );
	}
	else
	{
		m_currentValue = m_value;
	}
	*in = m_currentValue;
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2RotationAdapter::GetValueAt( Quaternion* in, Be::Time time )
{
	if( m_start == 0 )
	{
		m_start = time;
	}
	if( m_curve )
	{
		m_curve->GetValueAt( in, GetLocalTime( time ) );
	}
	else
	{
		*in = m_value;
	}
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2RotationAdapter::GetValueAt( Quaternion* in, double time )
{
	if( m_curve )
	{
		m_curve->GetValueAt( in, GetLocalTime( time ) );
	}
	else
	{
		*in = m_value;
	}
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2RotationAdapter::GetValueDotAt( Quaternion* in, Be::Time time )
{
	*in = Quaternion( 0, 0, 0, 1 );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2RotationAdapter::GetValueDotAt( Quaternion* in, double time )
{
	*in = Quaternion( 0, 0, 0, 1 );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2RotationAdapter::GetValueDoubleDotAt( Quaternion* in, Be::Time )
{
	*in = Quaternion( 0, 0, 0, 1 );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2RotationAdapter::GetValueDoubleDotAt( Quaternion* in, double )
{
	*in = Quaternion( 0, 0, 0, 1 );
	return in;
}

// --------------------------------------------------------------------------------
void Tr2RotationAdapter::RandomizeStart( float range )
{
	if( !range )
	{
		range = 60.f;
	}
	Be::Time trange = TimeFromDouble( range );
	m_offset = rand() % int( trange * 2 ) - trange;
}

// --------------------------------------------------------------------------------
void Tr2RotationAdapter::ScaleTime( float scale )
{
	m_timeScale = scale;
}

// --------------------------------------------------------------------------------
double Tr2RotationAdapter::GetLocalTime( double time ) const
{
	return time / m_timeScale;
}

// --------------------------------------------------------------------------------
double Tr2RotationAdapter::GetLocalTime( Be::Time time ) const
{
	return TimeAsDouble( time - m_start + m_offset ) / m_timeScale;
}

// --------------------------------------------------------------------------------
void Tr2RotationAdapter::ResetStart()
{
	m_start = 0;
}