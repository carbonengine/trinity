////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//

#pragma once

#include "Include/ITriFunction.h"
#include "Include/ITriCurveLength.h"
#include "Tr2CurveScalar.h"

namespace Tr2CurveVector3LerpKeyInterpolation
{
enum Type
{
	// Linear interpolation
	LINEAR = 1,
	// Hermite/cubic interpolation
	HERMITE = 2,
};
}

BLUE_CLASS( Tr2CurveVector3Lerp ) :
	public ITriVectorFunction
{
public:
	Tr2CurveVector3Lerp( IRoot* lockobj = nullptr );
	~Tr2CurveVector3Lerp();

	EXPOSE_TO_BLUE();

	virtual void UpdateValue( double time );

	Vector3 GetValue( double time ) const;

	virtual Vector3* Update( Vector3* in, Be::Time time );
	virtual Vector3* Update( Vector3* in, double time );
	virtual Vector3* GetValueAt( Vector3* in, Be::Time time );
	virtual Vector3* GetValueAt( Vector3* in, double time );
	virtual Vector3* GetValueDotAt( Vector3* in, Be::Time time );
	virtual Vector3* GetValueDotAt( Vector3* in, double time );
	virtual Vector3* GetValueDoubleDotAt( Vector3* in, Be::Time time );
	virtual Vector3* GetValueDoubleDotAt( Vector3* in, double time );
	virtual Vector3d* InterpolatedPosition( Vector3d* out, Be::Time time );
private:
	Vector3 LerpToFirstKey( double time ) const;
	Vector3 GetAutoTangent() const;

	std::string m_name;

	Vector3 m_initialValue;
	Vector3 m_currentValue;

	float m_curveStartTime;

	Tr2CurveVector3LerpKeyInterpolation::Type m_startInterpolation;

	ITriVectorFunctionPtr m_curve;

};

TYPEDEF_BLUECLASS( Tr2CurveVector3Lerp );