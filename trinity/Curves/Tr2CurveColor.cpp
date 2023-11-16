////////////////////////////////////////////////////////////
//
//    Created:   May 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "Tr2CurveColor.h"
#include "Tr2CurveScalar.h"
#include "TriUtil.h"


// --------------------------------------------------------------------------------
Tr2CurveColor::Tr2CurveColor( IRoot* lockobj )
	:PARENTLOCK( m_r ),
	PARENTLOCK( m_g ),
	PARENTLOCK( m_b ),
	PARENTLOCK( m_a ),
	m_timeOffset( 0.f ),
	m_currentValue( 0, 0, 0, 1 ),
	m_srgbOutput( false )
{
}

// --------------------------------------------------------------------------------
void Tr2CurveColor::UpdateValue( double time )
{
	double t = time - m_timeOffset;
	m_currentValue.r = m_r.Update( t );
	m_currentValue.g = m_g.Update( t );
	m_currentValue.b = m_b.Update( t );
	m_currentValue.a = m_a.Update( t );
	if( m_a.IsEmpty() )
	{
		m_currentValue.a = 1;
	}
	if( m_srgbOutput )
	{
		m_currentValue = TriLinearToGamma( m_currentValue );
	}
}

// --------------------------------------------------------------------------------
float Tr2CurveColor::Length()
{
	return std::max( std::max( std::max( m_r.Length(), m_g.Length() ), m_b.Length() ), m_a.Length() );
}

// --------------------------------------------------------------------------------
Color Tr2CurveColor::GetValue( double time ) const
{
	double t = time - m_timeOffset;
	Color color( m_r.GetValue( t ), m_g.GetValue( t ), m_b.GetValue( t ), m_a.IsEmpty() ? 1.0f : m_a.GetValue( t ) );
	if( m_srgbOutput )
	{
		color.r = std::max( color.r, 0.f );
		color.g = std::max( color.g, 0.f );
		color.b = std::max( color.b, 0.f );
		color.a = std::max( color.a, 0.f );
		color = TriLinearToGamma( color );
	}
	return color;
}

// --------------------------------------------------------------------------------
Color* Tr2CurveColor::Update( Color* in, Be::Time time )
{
	*in = m_currentValue = GetValue( TimeAsDouble( time ) );
	return in;
}

// --------------------------------------------------------------------------------
Color* Tr2CurveColor::Update( Color* in, double time )
{
	*in = m_currentValue = GetValue( time );
	return in;
}

// --------------------------------------------------------------------------------
Color* Tr2CurveColor::GetValueAt( Color* in, Be::Time time )
{
	*in = GetValue( TimeAsDouble( time ) );
	return in;
}

// --------------------------------------------------------------------------------
Color* Tr2CurveColor::GetValueAt( Color* in, double time )
{
	*in = GetValue( time );
	return in;
}

// --------------------------------------------------------------------------------
void Tr2CurveColor::AddKey(
	float time,
	Color value,
	Be::OptionalWithDefaultValue<Tr2CurveInterpolation::Type, Tr2CurveInterpolation::HERMITE> interpolation,
	Be::Optional<Color> leftTangent,
	Be::Optional<Color> rightTangent,
	Be::OptionalWithDefaultValue<Tr2CurveTangentType::Type, Tr2CurveTangentType::AUTO_CLAMP> tangentType )
{
	Color lt = leftTangent.IsAssigned() ? Color( leftTangent ) : Color( 0, 0, 0, 0 );
	Color rt = leftTangent.IsAssigned() ? Color( rightTangent ) : Color( 0, 0, 0, 0 );
	m_r.AddKey( time, value.r, interpolation, lt.r, rt.r, tangentType );
	m_g.AddKey( time, value.g, interpolation, lt.g, rt.g, tangentType );
	m_b.AddKey( time, value.b, interpolation, lt.b, rt.b, tangentType );
	m_a.AddKey( time, value.a, interpolation, lt.a, rt.a, tangentType );
}

// --------------------------------------------------------------------------------
void Tr2CurveColor::SetExtrapolation( Tr2CurveExtrapolation::Type extrapolation )
{
	m_r.SetExtrapolation( extrapolation );
	m_g.SetExtrapolation( extrapolation );
	m_b.SetExtrapolation( extrapolation );
	m_a.SetExtrapolation( extrapolation );
}
