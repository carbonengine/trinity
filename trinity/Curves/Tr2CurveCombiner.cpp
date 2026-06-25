// Copyright © 2022 CCP ehf.

#include "Tr2CurveCombiner.h"


Tr2CurveCombiner::Tr2CurveCombiner( IRoot* lockobj ) :
	PARENTLOCK( m_curves ),
	m_currentValue( 0, 0, 0 )
{
}

void Tr2CurveCombiner::UpdateValue( double time )
{
	Vector3 out( 0.f, 0.f, 0.f );
	for( auto it = begin( m_curves ); it != end( m_curves ); ++it )
	{
		Vector3 temp( 0.f, 0.f, 0.f );
		( *it )->Update( &temp, time );
		out += temp;
	}
	m_currentValue = out;
}

float Tr2CurveCombiner::Length()
{
	float maxLength = 0.f;
	for( auto it = begin( m_curves ); it != end( m_curves ); ++it )
	{
		if( ITriCurveLengthPtr curve = BlueCastPtr( *it ) )
		{
			maxLength = max( curve->Length(), maxLength );
		}
	}
	return maxLength;
}

Vector3 Tr2CurveCombiner::GetValue( double time ) const
{
	Vector3 out( 0.f, 0.f, 0.f );
	for( auto it = begin( m_curves ); it != end( m_curves ); ++it )
	{
		Vector3 temp( 0.f, 0.f, 0.f );
		( *it )->GetValueAt( &temp, time );
		out += temp;
	}
	return out;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveCombiner::Update( Vector3* in, Be::Time time )
{
	Vector3 out( 0.f, 0.f, 0.f );
	for( auto it = begin( m_curves ); it != end( m_curves ); ++it )
	{
		Vector3 temp( 0.f, 0.f, 0.f );
		( *it )->Update( &temp, time );
		out += temp;
	}
	m_currentValue = out;
	*in = m_currentValue;
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveCombiner::Update( Vector3* in, double time )
{
	Vector3 out( 0.f, 0.f, 0.f );
	for( auto it = begin( m_curves ); it != end( m_curves ); ++it )
	{
		Vector3 temp( 0.f, 0.f, 0.f );
		( *it )->Update( &temp, time );
		out += temp;
	}
	m_currentValue = out;
	*in = m_currentValue;
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveCombiner::GetValueAt( Vector3* in, Be::Time time )
{
	*in = GetValue( TimeAsDouble( time ) );
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveCombiner::GetValueAt( Vector3* in, double time )
{
	*in = GetValue( time );
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveCombiner::GetValueDotAt( Vector3* in, Be::Time )
{
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveCombiner::GetValueDotAt( Vector3* in, double )
{
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveCombiner::GetValueDoubleDotAt( Vector3* in, Be::Time )
{
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveCombiner::GetValueDoubleDotAt( Vector3* in, double )
{
	return in;
}

// --------------------------------------------------------------------------------
Vector3d* Tr2CurveCombiner::InterpolatedPosition( Vector3d* out, Be::Time )
{
	return out;
}