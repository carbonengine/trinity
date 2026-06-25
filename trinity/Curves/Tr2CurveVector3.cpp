// Copyright © 2017 CCP ehf.

#include "StdAfx.h"
#include "Tr2CurveVector3.h"
#include "Tr2CurveScalar.h"


Tr2CurveVector3::Tr2CurveVector3( IRoot* lockobj ) :
	PARENTLOCK( m_x ),
	PARENTLOCK( m_y ),
	PARENTLOCK( m_z ),
	m_currentValue( 0, 0, 0 )
{
}

void Tr2CurveVector3::UpdateValue( double time )
{
	m_currentValue.x = m_x.Update( time );
	m_currentValue.y = m_y.Update( time );
	m_currentValue.z = m_z.Update( time );
}

float Tr2CurveVector3::Length()
{
	return std::max( std::max( m_x.Length(), m_y.Length() ), m_z.Length() );
}

Vector3 Tr2CurveVector3::GetValue( double time ) const
{
	return Vector3( m_x.GetValue( time ), m_y.GetValue( time ), m_z.GetValue( time ) );
}

// --------------------------------------------------------------------------------
void Tr2CurveVector3::AddKey(
	float time,
	Vector3 value,
	Be::OptionalWithDefaultValue<Tr2CurveInterpolation::Type, Tr2CurveInterpolation::HERMITE> interpolation,
	Be::Optional<Vector3> leftTangent,
	Be::Optional<Vector3> rightTangent,
	Be::OptionalWithDefaultValue<Tr2CurveTangentType::Type, Tr2CurveTangentType::AUTO_CLAMP> tangentType )
{
	auto lt = leftTangent.IsAssigned() ? Vector3( leftTangent ) : Vector3( 0, 0, 0 );
	auto rt = leftTangent.IsAssigned() ? Vector3( rightTangent ) : Vector3( 0, 0, 0 );
	m_x.AddKey( time, value.x, interpolation, lt.x, rt.x, tangentType );
	m_y.AddKey( time, value.y, interpolation, lt.y, rt.y, tangentType );
	m_z.AddKey( time, value.z, interpolation, lt.z, rt.z, tangentType );
}

// --------------------------------------------------------------------------------
void Tr2CurveVector3::SetExtrapolation( Tr2CurveExtrapolation::Type extrapolation )
{
	m_x.SetExtrapolation( extrapolation );
	m_y.SetExtrapolation( extrapolation );
	m_z.SetExtrapolation( extrapolation );
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveVector3::Update( Vector3* in, Be::Time time )
{
	m_currentValue = GetValue( TimeAsDouble( time ) );
	*in = m_currentValue;
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveVector3::Update( Vector3* in, double time )
{
	m_currentValue = GetValue( time );
	*in = m_currentValue;
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveVector3::GetValueAt( Vector3* in, Be::Time time )
{
	*in = GetValue( TimeAsDouble( time ) );
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveVector3::GetValueAt( Vector3* in, double time )
{
	*in = GetValue( time );
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveVector3::GetValueDotAt( Vector3* in, Be::Time )
{
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveVector3::GetValueDotAt( Vector3* in, double )
{
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveVector3::GetValueDoubleDotAt( Vector3* in, Be::Time )
{
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveVector3::GetValueDoubleDotAt( Vector3* in, double )
{
	return in;
}

// --------------------------------------------------------------------------------
Vector3d* Tr2CurveVector3::InterpolatedPosition( Vector3d* out, Be::Time )
{
	return out;
}