// Copyright © 2021 CCP ehf.

#pragma once

#include "Tr2CurveScalar.h"

#include <ITriFunction.h>
#include <ITriCurveLength.h>


BLUE_CLASS( Tr2CurveColorMixer ) :
	public ITriCurveLength,
	public ITriColorFunction
{
public:
	Tr2CurveColorMixer( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	Color GetValue( double time ) const;

	// ITriColorFunction
	void UpdateValue( double time ) override;
	Color* Update( Color * in, Be::Time time ) override;
	Color* Update( Color * in, double time ) override;
	Color* GetValueAt( Color * in, Be::Time time ) override;
	Color* GetValueAt( Color * in, double time ) override;

	// ITriCurveLength
	float Length() override;

private:
	// this function was added as an option to convert Linear color to SRGB Color
	void InvertLinearColor( Color * in, Color * out );
	float InvertLinearValue( float x );

	std::string m_name;

	Color m_convertedLinearValue;
	Color m_color1;
	Color m_color2;
	Color m_currentValue;

	float m_lerpValue;
	float m_saturation;
	float m_brightness;
};

TYPEDEF_BLUECLASS( Tr2CurveColorMixer );