// Copyright © 2017 CCP ehf.

#pragma once

#include <ITriFunction.h>

BLUE_CLASS( Tr2TranslationAdapter ) :
	public ITriVectorFunction
{
public:
	Tr2TranslationAdapter( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual void UpdateValue( double time );
	virtual Vector3* Update( Vector3 * in, Be::Time time );
	virtual Vector3* Update( Vector3 * in, double time );
	virtual Vector3* GetValueAt( Vector3 * in, Be::Time time );
	virtual Vector3* GetValueAt( Vector3 * in, double time );
	virtual Vector3* GetValueDotAt( Vector3 * in, Be::Time time );
	virtual Vector3* GetValueDotAt( Vector3 * in, double time );
	virtual Vector3* GetValueDoubleDotAt( Vector3 * in, Be::Time time );
	virtual Vector3* GetValueDoubleDotAt( Vector3 * in, double time );
	virtual Vector3d* InterpolatedPosition( Vector3d * out, Be::Time time );

	void RandomizeStart( float range );
	void ScaleTime( float scale );
	void ResetStart();

private:
	double GetLocalTime( double time ) const;
	double GetLocalTime( Be::Time time ) const;

	Be::Time m_start;
	Be::Time m_offset;
	Quaternion m_rotationOffset;
	ITriVectorFunctionPtr m_curve;
	Vector3 m_value;
	Vector3 m_currentValue;
	float m_timeScale;
};

TYPEDEF_BLUECLASS( Tr2TranslationAdapter );
