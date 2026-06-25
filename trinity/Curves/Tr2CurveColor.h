// Copyright © 2017 CCP ehf.

#pragma once

#include "Tr2CurveScalar.h"

#include <ITriFunction.h>
#include <ITriCurveLength.h>

BLUE_CLASS( Tr2CurveColor ) :
	public ITriCurveLength,
	public ITriColorFunction
{
public:
	Tr2CurveColor( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual void UpdateValue( double time );

	virtual Color* Update( Color * in, Be::Time time );
	virtual Color* Update( Color * in, double time );
	virtual Color* GetValueAt( Color * in, Be::Time time );
	virtual Color* GetValueAt( Color * in, double time );

	virtual float Length();

	Color GetValue( double time ) const;

	void AddKey(
		float time,
		Color value,
		Be::OptionalWithDefaultValue<Tr2CurveInterpolation::Type, Tr2CurveInterpolation::HERMITE> interpolation,
		Be::Optional<Color> leftTangent,
		Be::Optional<Color> rightTangent,
		Be::OptionalWithDefaultValue<Tr2CurveTangentType::Type, Tr2CurveTangentType::AUTO_CLAMP> tangentType );

	void SetExtrapolation( Tr2CurveExtrapolation::Type extrapolation );

	PTr2CurveScalar m_r;
	PTr2CurveScalar m_g;
	PTr2CurveScalar m_b;
	PTr2CurveScalar m_a;

private:
	std::string m_name;

	Color m_currentValue;

	float m_timeOffset;

	bool m_srgbOutput;
};

TYPEDEF_BLUECLASS( Tr2CurveColor );