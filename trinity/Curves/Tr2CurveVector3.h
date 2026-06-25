// Copyright © 2017 CCP ehf.

#pragma once

#include "Tr2CurveScalar.h"

#include <ITriFunction.h>
#include <ITriCurveLength.h>


BLUE_CLASS( Tr2CurveVector3 ) :
	public ITriCurveLength,
	public ITriVectorFunction
{
public:
	Tr2CurveVector3( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual void UpdateValue( double time );

	virtual float Length();

	Vector3 GetValue( double time ) const;

	void AddKey(
		float time,
		Vector3 value,
		Be::OptionalWithDefaultValue<Tr2CurveInterpolation::Type, Tr2CurveInterpolation::HERMITE> interpolation,
		Be::Optional<Vector3> leftTangent,
		Be::Optional<Vector3> rightTangent,
		Be::OptionalWithDefaultValue<Tr2CurveTangentType::Type, Tr2CurveTangentType::AUTO_CLAMP> tangentType );

	void SetExtrapolation( Tr2CurveExtrapolation::Type extrapolation );

	virtual Vector3* Update( Vector3 * in, Be::Time time );
	virtual Vector3* Update( Vector3 * in, double time );
	virtual Vector3* GetValueAt( Vector3 * in, Be::Time time );
	virtual Vector3* GetValueAt( Vector3 * in, double time );
	virtual Vector3* GetValueDotAt( Vector3 * in, Be::Time time );
	virtual Vector3* GetValueDotAt( Vector3 * in, double time );
	virtual Vector3* GetValueDoubleDotAt( Vector3 * in, Be::Time time );
	virtual Vector3* GetValueDoubleDotAt( Vector3 * in, double time );
	virtual Vector3d* InterpolatedPosition( Vector3d * out, Be::Time time );

private:
	std::string m_name;

	PTr2CurveScalar m_x;
	PTr2CurveScalar m_y;
	PTr2CurveScalar m_z;

	Vector3 m_currentValue;
};

TYPEDEF_BLUECLASS( Tr2CurveVector3 );