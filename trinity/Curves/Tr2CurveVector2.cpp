// Copyright © 2017 CCP ehf.

#include "StdAfx.h"
#include "Tr2CurveVector2.h"
#include "Tr2CurveScalar.h"


// --------------------------------------------------------------------------------
Tr2CurveVector2::Tr2CurveVector2( IRoot* lockobj ) :
	PARENTLOCK( m_x ),
	PARENTLOCK( m_y ),
	m_currentValue( 0, 0 )
{
}

// --------------------------------------------------------------------------------
void Tr2CurveVector2::UpdateValue( double time )
{
	m_currentValue.x = m_x.Update( time );
	m_currentValue.y = m_y.Update( time );
}

// --------------------------------------------------------------------------------
float Tr2CurveVector2::Length()
{
	return std::max( m_x.Length(), m_y.Length() );
}

// --------------------------------------------------------------------------------
Vector2 Tr2CurveVector2::GetValue( double time ) const
{
	return Vector2( m_x.GetValue( time ), m_y.GetValue( time ) );
}

// --------------------------------------------------------------------------------
void Tr2CurveVector2::AddKey(
	float time,
	Vector2 value,
	Be::OptionalWithDefaultValue<Tr2CurveInterpolation::Type, Tr2CurveInterpolation::HERMITE> interpolation,
	Be::Optional<Vector2> leftTangent,
	Be::Optional<Vector2> rightTangent,
	Be::OptionalWithDefaultValue<Tr2CurveTangentType::Type, Tr2CurveTangentType::AUTO_CLAMP> tangentType )
{
	auto lt = leftTangent.IsAssigned() ? Vector2( leftTangent ) : Vector2( 0, 0 );
	auto rt = leftTangent.IsAssigned() ? Vector2( rightTangent ) : Vector2( 0, 0 );
	m_x.AddKey( time, value.x, interpolation, lt.x, rt.x, tangentType );
	m_y.AddKey( time, value.y, interpolation, lt.y, rt.y, tangentType );
}

// --------------------------------------------------------------------------------
void Tr2CurveVector2::SetExtrapolation( Tr2CurveExtrapolation::Type extrapolation )
{
	m_x.SetExtrapolation( extrapolation );
	m_y.SetExtrapolation( extrapolation );
}
