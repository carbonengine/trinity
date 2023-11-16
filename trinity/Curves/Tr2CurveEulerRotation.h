////////////////////////////////////////////////////////////
//
//    Created:   May 2017
//    Copyright: CCP 2017
//

#pragma once

#include "Include/ITriFunction.h"
#include "Include/ITriCurveLength.h"
#include "Tr2CurveScalar.h"


BLUE_CLASS( Tr2CurveEulerRotation ) :
	public ITriCurveLength,
	public ITriQuaternionFunction
{
public:
	Tr2CurveEulerRotation( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual void UpdateValue( double time );

	virtual Quaternion* Update( Quaternion* in, Be::Time time );
	virtual Quaternion* Update( Quaternion* in, double time );
	virtual Quaternion* GetValueAt( Quaternion* in, Be::Time time );
	virtual Quaternion* GetValueAt( Quaternion* in, double time );
	virtual Quaternion* GetValueDotAt( Quaternion* in, Be::Time time );
	virtual Quaternion* GetValueDotAt( Quaternion* in, double time );
	virtual Quaternion* GetValueDoubleDotAt( Quaternion* in, Be::Time time );
	virtual Quaternion* GetValueDoubleDotAt( Quaternion* in, double time );

	virtual float Length();

	Quaternion GetValue( double time ) const;

	void AddKey(
		float time,
		Vector3 value,
		Be::OptionalWithDefaultValue<Tr2CurveInterpolation::Type, Tr2CurveInterpolation::HERMITE> interpolation,
		Be::Optional<Vector3> leftTangent,
		Be::Optional<Vector3> rightTangent,
		Be::OptionalWithDefaultValue<Tr2CurveTangentType::Type, Tr2CurveTangentType::AUTO_CLAMP> tangentType );

	void SetExtrapolation( Tr2CurveExtrapolation::Type extrapolation );
private:
	std::string m_name;

	PTr2CurveScalar m_yaw;
	PTr2CurveScalar m_pitch;
	PTr2CurveScalar m_roll;

	Quaternion m_currentValue;
};

TYPEDEF_BLUECLASS( Tr2CurveEulerRotation );