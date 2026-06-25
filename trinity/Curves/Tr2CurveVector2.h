// Copyright © 2017 CCP ehf.

#pragma once

#include "Tr2CurveScalar.h"

#include <ITriFunction.h>
#include <ITriCurveLength.h>


BLUE_CLASS( Tr2CurveVector2 ) :
	public ITriCurveLength,
	public ITriFunction
{
public:
	Tr2CurveVector2( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual void UpdateValue( double time );

	virtual float Length();

	Vector2 GetValue( double time ) const;

	void AddKey(
		float time,
		Vector2 value,
		Be::OptionalWithDefaultValue<Tr2CurveInterpolation::Type, Tr2CurveInterpolation::HERMITE> interpolation,
		Be::Optional<Vector2> leftTangent,
		Be::Optional<Vector2> rightTangent,
		Be::OptionalWithDefaultValue<Tr2CurveTangentType::Type, Tr2CurveTangentType::AUTO_CLAMP> tangentType );

	void SetExtrapolation( Tr2CurveExtrapolation::Type extrapolation );

private:
	std::string m_name;

	PTr2CurveScalar m_x;
	PTr2CurveScalar m_y;

	Vector2 m_currentValue;
};

TYPEDEF_BLUECLASS( Tr2CurveVector2 );
