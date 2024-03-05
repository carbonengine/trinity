//    Created:   Feb 2022
//    Copyright: CCP 2022


#pragma once

#include "Include/ITriFunction.h"
#include "Include/ITriCurveLength.h"
#include "Tr2CurveScalar.h"

BLUE_DECLARE_INTERFACE( ITriVectorFunction );
BLUE_DECLARE_IVECTOR( ITriVectorFunction );

BLUE_CLASS( Tr2CurveCombiner ):
	public ITriCurveLength,
	public ITriVectorFunction
{
public:
	Tr2CurveCombiner( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual void UpdateValue( double time );

	virtual float Length();

	Vector3 GetValue( double time ) const;

	Vector3* Update( Vector3* in, Be::Time time ) override;
	Vector3* Update( Vector3* in, double time ) override;
	Vector3* GetValueAt( Vector3* in, Be::Time time ) override;
	Vector3* GetValueAt( Vector3* in, double time ) override;
	Vector3* GetValueDotAt( Vector3* in, Be::Time time ) override;
	Vector3* GetValueDotAt( Vector3* in, double time ) override;
	Vector3* GetValueDoubleDotAt( Vector3* in, Be::Time time ) override;
	Vector3* GetValueDoubleDotAt( Vector3* in, double time ) override;
	Vector3d* InterpolatedPosition( Vector3d* out, Be::Time time ) override;
private:
	std::string m_name;
	PITriVectorFunctionVector m_curves;
	Vector3 m_currentValue;
};

TYPEDEF_BLUECLASS( Tr2CurveCombiner );