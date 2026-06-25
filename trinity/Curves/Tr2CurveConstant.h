// Copyright © 2017 CCP ehf.

#pragma once

#include <ITriFunction.h>

BLUE_CLASS( Tr2CurveConstant ) :
	public ITriScalarFunction,
	public ITriVectorFunction,
	public ITriQuaternionFunction,
	public ITriColorFunction
{
public:
	Tr2CurveConstant( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual void UpdateValue( double time );

	virtual float Update( Be::Time time );
	virtual float Update( double time );
	virtual float GetValueAt( Be::Time time );
	virtual float GetValueAt( double time );
	virtual void ScaleTime( float s );

	virtual Vector3* Update( Vector3 * in, Be::Time time );
	virtual Vector3* Update( Vector3 * in, double time );
	virtual Vector3* GetValueAt( Vector3 * in, Be::Time time );
	virtual Vector3* GetValueAt( Vector3 * in, double time );
	virtual Vector3* GetValueDotAt( Vector3 * in, Be::Time time );
	virtual Vector3* GetValueDotAt( Vector3 * in, double time );
	virtual Vector3* GetValueDoubleDotAt( Vector3 * in, Be::Time time );
	virtual Vector3* GetValueDoubleDotAt( Vector3 * in, double time );
	virtual Vector3d* InterpolatedPosition( Vector3d * out, Be::Time time );

	virtual Quaternion* Update( Quaternion * in, Be::Time time );
	virtual Quaternion* Update( Quaternion * in, double time );
	virtual Quaternion* GetValueAt( Quaternion * in, Be::Time time );
	virtual Quaternion* GetValueAt( Quaternion * in, double time );
	virtual Quaternion* GetValueDotAt( Quaternion * in, Be::Time time );
	virtual Quaternion* GetValueDotAt( Quaternion * in, double time );
	virtual Quaternion* GetValueDoubleDotAt( Quaternion * in, Be::Time time );
	virtual Quaternion* GetValueDoubleDotAt( Quaternion * in, double time );

	virtual Color* Update( Color * in, Be::Time time );
	virtual Color* Update( Color * in, double time );
	virtual Color* GetValueAt( Color * in, Be::Time time );
	virtual Color* GetValueAt( Color * in, double time );

private:
	std::string m_name;
	Vector4 m_value;
};

TYPEDEF_BLUECLASS( Tr2CurveConstant );