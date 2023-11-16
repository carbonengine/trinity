////////////////////////////////////////////////////////////
//
//    Created:   May 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "Tr2CurveEulerRotation.h"
#include "Tr2CurveScalar.h"


Tr2CurveEulerRotation::Tr2CurveEulerRotation( IRoot* lockobj )
	:PARENTLOCK( m_yaw ),
	PARENTLOCK( m_pitch ),
	PARENTLOCK( m_roll ),
	m_currentValue( 0, 0, 0, 1 )
{
}

void Tr2CurveEulerRotation::UpdateValue( double time )
{
	float yaw = m_yaw.Update( time );
	float pitch = m_pitch.Update( time );
	float roll = m_roll.Update( time );
    m_currentValue = RotationQuaternion( yaw, pitch, roll );
}

float Tr2CurveEulerRotation::Length()
{
	return std::max( std::max( m_yaw.Length(), m_pitch.Length() ), m_roll.Length() );
}

Quaternion Tr2CurveEulerRotation::GetValue( double time ) const
{
	float yaw = m_yaw.GetValue( time );
	float pitch = m_pitch.GetValue( time );
	float roll = m_roll.GetValue( time );
    return RotationQuaternion( yaw, pitch, roll );
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveEulerRotation::Update( Quaternion* in, Be::Time time )
{
	*in = m_currentValue = GetValue( TimeAsDouble( time ) );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveEulerRotation::Update( Quaternion* in, double time )
{
	*in = m_currentValue = GetValue( time );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveEulerRotation::GetValueAt( Quaternion* in, Be::Time time )
{
	*in = GetValue( TimeAsDouble( time ) );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveEulerRotation::GetValueAt( Quaternion* in, double time )
{
	*in = GetValue( time );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveEulerRotation::GetValueDotAt( Quaternion* in, Be::Time time )
{
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveEulerRotation::GetValueDotAt( Quaternion* in, double time )
{
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveEulerRotation::GetValueDoubleDotAt( Quaternion* in, Be::Time time )
{
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveEulerRotation::GetValueDoubleDotAt( Quaternion* in, double time )
{
	return in;
}

// --------------------------------------------------------------------------------
void Tr2CurveEulerRotation::AddKey(
	float time,
	Vector3 value,
	Be::OptionalWithDefaultValue<Tr2CurveInterpolation::Type, Tr2CurveInterpolation::HERMITE> interpolation,
	Be::Optional<Vector3> leftTangent,
	Be::Optional<Vector3> rightTangent,
	Be::OptionalWithDefaultValue<Tr2CurveTangentType::Type, Tr2CurveTangentType::AUTO_CLAMP> tangentType )
{
	auto lt = leftTangent.IsAssigned() ? Vector3( leftTangent ) : Vector3( 0, 0, 0 );
	auto rt = leftTangent.IsAssigned() ? Vector3( rightTangent ) : Vector3( 0, 0, 0 );
	m_yaw.AddKey( time, value.x, interpolation, lt.x, rt.x, tangentType );
	m_pitch.AddKey( time, value.y, interpolation, lt.y, rt.y, tangentType );
	m_roll.AddKey( time, value.z, interpolation, lt.z, rt.z, tangentType );
}

// --------------------------------------------------------------------------------
void Tr2CurveEulerRotation::SetExtrapolation( Tr2CurveExtrapolation::Type extrapolation )
{
	m_yaw.SetExtrapolation( extrapolation );
	m_pitch.SetExtrapolation( extrapolation );
	m_roll.SetExtrapolation( extrapolation );
}
