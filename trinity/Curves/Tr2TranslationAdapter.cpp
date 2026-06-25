// Copyright © 2017 CCP ehf.

#include "StdAfx.h"
#include "Tr2TranslationAdapter.h"
#include "Utilities/Vector3d.h"
#include "Include/TriMath.h"


// --------------------------------------------------------------------------------
Tr2TranslationAdapter::Tr2TranslationAdapter( IRoot* ) :
	m_start( 0 ),
	m_offset( 0 ),
	m_value( 0, 0, 0 ),
	m_currentValue( 0, 0, 0 ),
	m_rotationOffset( 0, 0, 0, 1 ),
	m_timeScale( 1 )
{
}

// --------------------------------------------------------------------------------
void Tr2TranslationAdapter::UpdateValue( double time )
{
	if( m_curve )
	{
		m_curve->Update( &m_currentValue, GetLocalTime( time ) );
	}
}

// --------------------------------------------------------------------------------
Vector3* Tr2TranslationAdapter::Update( Vector3* in, Be::Time time )
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

	TriVectorRotateQuaternion( &m_currentValue, &m_currentValue, &m_rotationOffset );

	*in = m_currentValue;
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2TranslationAdapter::Update( Vector3* in, double time )
{
	if( m_curve )
	{
		m_curve->Update( &m_currentValue, GetLocalTime( time ) );
	}
	else
	{
		m_currentValue = m_value;
	}

	TriVectorRotateQuaternion( &m_currentValue, &m_currentValue, &m_rotationOffset );

	*in = m_currentValue;
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2TranslationAdapter::GetValueAt( Vector3* in, Be::Time time )
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
Vector3* Tr2TranslationAdapter::GetValueAt( Vector3* in, double time )
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
Vector3* Tr2TranslationAdapter::GetValueDotAt( Vector3* in, Be::Time time )
{
	if( m_start == 0 )
	{
		m_start = time;
	}
	if( m_curve )
	{
		Vector3 v0, v1;
		m_curve->GetValueAt( &v0, GetLocalTime( time ) );
		m_curve->GetValueAt( &v1, GetLocalTime( time ) - 0.1 );
		*in = Vector3( ( v1 - v0 ) / 0.1f );
	}
	else
	{
		*in = Vector3( 0, 0, 0 );
	}
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2TranslationAdapter::GetValueDotAt( Vector3* in, double time )
{
	if( m_curve )
	{
		Vector3 v0, v1;
		m_curve->GetValueAt( &v0, GetLocalTime( time ) );
		m_curve->GetValueAt( &v1, GetLocalTime( time ) - 0.1 );
		*in = Vector3( ( v1 - v0 ) / 0.1f );
	}
	else
	{
		*in = Vector3( 0, 0, 0 );
	}
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2TranslationAdapter::GetValueDoubleDotAt( Vector3* in, Be::Time )
{
	*in = Vector3( 0, 0, 0 );
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2TranslationAdapter::GetValueDoubleDotAt( Vector3* in, double )
{
	*in = Vector3( 0, 0, 0 );
	return in;
}

// --------------------------------------------------------------------------------
Vector3d* Tr2TranslationAdapter::InterpolatedPosition( Vector3d* out, Be::Time )
{
	out->x = m_currentValue.x;
	out->y = m_currentValue.y;
	out->z = m_currentValue.z;
	return out;
}

// --------------------------------------------------------------------------------
void Tr2TranslationAdapter::RandomizeStart( float range )
{
	if( !range )
	{
		range = 60.f;
	}
	Be::Time trange = TimeFromDouble( range );
	m_offset = rand() % int( trange * 2 ) - trange;
}

// --------------------------------------------------------------------------------
void Tr2TranslationAdapter::ScaleTime( float scale )
{
	m_timeScale = scale;
}

// --------------------------------------------------------------------------------
double Tr2TranslationAdapter::GetLocalTime( double time ) const
{
	return time / m_timeScale;
}

// --------------------------------------------------------------------------------
double Tr2TranslationAdapter::GetLocalTime( Be::Time time ) const
{
	return TimeAsDouble( time - m_start + m_offset ) / m_timeScale;
}

// --------------------------------------------------------------------------------
void Tr2TranslationAdapter::ResetStart()
{
	m_start = 0;
}