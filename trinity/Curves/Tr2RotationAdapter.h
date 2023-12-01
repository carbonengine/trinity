////////////////////////////////////////////////////////////
//
//    Created:   June 2017
//    Copyright: CCP 2017
//

#pragma once

#include "Include/ITriFunction.h"

BLUE_CLASS( Tr2RotationAdapter ) : public ITriQuaternionFunction
{
public:
	Tr2RotationAdapter( IRoot* lockobj = nullptr );

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

	void RandomizeStart( float range );
	void ScaleTime( float scale );
	void ResetStart();
private:
	double GetLocalTime( double time ) const;
	double GetLocalTime( Be::Time time ) const;

	Be::Time m_start;
	Be::Time m_offset;
	ITriQuaternionFunctionPtr m_curve;
	Quaternion m_value;
	Quaternion m_currentValue;

	float m_timeScale;
};

TYPEDEF_BLUECLASS( Tr2RotationAdapter );
