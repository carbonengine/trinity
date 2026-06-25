// Copyright © 2023 CCP ehf.

#include "Tr2CurveColorMixer.h"

Tr2CurveColorMixer::Tr2CurveColorMixer( IRoot* lockobj ) :
	m_currentValue( 0, 0, 0, 1 ),
	m_convertedLinearValue( 0, 0, 0, 1 ),
	m_color1( 0, 0, 0, 1 ),
	m_color2( 0, 0, 0, 1 ),
	m_lerpValue( 0.f ),
	m_saturation( 1.f ),
	m_brightness( 1.f )
{
}

void Tr2CurveColorMixer::UpdateValue( double time )
{
	m_currentValue = GetValue( time );
	InvertLinearColor( &m_currentValue, &m_convertedLinearValue );
}

float Tr2CurveColorMixer::Length()
{
	return 0;
}

Color Tr2CurveColorMixer::GetValue( double time ) const
{
	Color out = Lerp( m_color1, m_color2, m_lerpValue );

	if( m_saturation == 1.f )
	{
		return out * m_brightness;
	}

	// color intensity
	float i = ( out.r * 0.299f ) + ( out.g * 0.587f ) + ( out.b * 0.114f );

	out = Lerp( Color( i, i, i, i ), out, max( 0.0f, m_saturation ) );

	return out * m_brightness;
}

Color* Tr2CurveColorMixer::Update( Color* in, Be::Time time )
{
	*in = m_currentValue = GetValue( TimeAsDouble( time ) );

	return in;
}

Color* Tr2CurveColorMixer::Update( Color* in, double time )
{
	*in = m_currentValue = GetValue( time );
	return in;
}

Color* Tr2CurveColorMixer::GetValueAt( Color* in, Be::Time time )
{
	*in = GetValue( TimeAsDouble( time ) );
	return in;
}

Color* Tr2CurveColorMixer::GetValueAt( Color* in, double time )
{
	*in = GetValue( time );
	return in;
}

void Tr2CurveColorMixer::InvertLinearColor( Color* in, Color* out )
{
	// converting the input color from linear to srgb
	out->r = InvertLinearValue( in->r );
	out->b = InvertLinearValue( in->b );
	out->g = InvertLinearValue( in->g );
}

float Tr2CurveColorMixer::InvertLinearValue( float x )
{
	if( x < 0.04045f )
	{
		return x / 12.92f;
	}

	return std::pow( ( x + 0.055f ) / 1.055f, 2.4f );
}
