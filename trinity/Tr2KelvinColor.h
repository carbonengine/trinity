// Copyright © 2023 CCP ehf.

#pragma once

#ifndef Tr2KelvinColor_h
#define Tr2KelvinColor_h

#include "Utilities/Vector3d.h"

// Standard Illuminants
enum Tr2StandardIlluminant
{
	TR2STANDARDILLUMINANT_A,
	TR2STANDARDILLUMINANT_D50,
	TR2STANDARDILLUMINANT_D55,
	TR2STANDARDILLUMINANT_D65,
	TR2STANDARDILLUMINANT_D75,
	TR2STANDARDILLUMINANT_E,
	TR2STANDARDILLUMINANT_F1,
	TR2STANDARDILLUMINANT_F2,
	TR2STANDARDILLUMINANT_F3,
	TR2STANDARDILLUMINANT_F4,
	TR2STANDARDILLUMINANT_F5,
	TR2STANDARDILLUMINANT_F6,
	TR2STANDARDILLUMINANT_F7,
	TR2STANDARDILLUMINANT_F8,
	TR2STANDARDILLUMINANT_F9,
	TR2STANDARDILLUMINANT_F10,
	TR2STANDARDILLUMINANT_F11,
	TR2STANDARDILLUMINANT_F12
};

// Converts kelvin color temperature to RGB
Vector3d TriColorFromKelvin( float temperature, float tint, Tr2StandardIlluminant whiteBalance );

BLUE_CLASS( Tr2KelvinColor ) :
	public IInitialize
{
public:
	Tr2KelvinColor( IRoot* lockobj = NULL );
	~Tr2KelvinColor();

	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	virtual bool Initialize();

	// Gets the current Kelvin color temperature
	float GetTemperature( void ) const
	{
		return m_temperature;
	}
	// Gets the current tint value
	float GetTint( void ) const
	{
		return m_tint;
	}
	// Gets the current white balance reference
	Tr2StandardIlluminant GetWhiteBalance( void ) const
	{
		return m_whiteBalance;
	}

	// Converts the color temperature to RGB
	Color AsRGB( void ) const
	{
		Vector3d col = TriColorFromKelvin( m_temperature, m_tint, m_whiteBalance );
		return Color(
			static_cast<float>( col.x ),
			static_cast<float>( col.y ),
			static_cast<float>( col.z ),
			1.0f );
	}

private:
	float m_temperature;
	float m_tint;
	Tr2StandardIlluminant m_whiteBalance;
};

TYPEDEF_BLUECLASS( Tr2KelvinColor );

#endif