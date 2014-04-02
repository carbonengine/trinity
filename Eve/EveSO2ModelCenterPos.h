/* 
*************************************************************************************

EveSO2ModelCenterPos.h

Created:   November 2007
OS:        Win32
Project:   Trinity

Description:   

Unfortunately the same as before. I need a working camera that doesn't disrupt the python code too much.

*************************************************************************************
*/
#ifndef _EVESPACEOBJECTMODELCENTERPOSITION_H_
#define _EVESPACEOBJECTMODELCENTERPOSITION_H_

#include "include/ITriFunction.h"
#include "IEveSpaceObject2.h"
#include "include/TriVector.h"

class EveSO2ModelCenterPos:
	public ITriVectorFunction
{

public:

	EXPOSE_TO_BLUE();

	std::wstring  mName;
	IEveSpaceObject2Ptr m_parentObject;
	Vector3 mValue;

	EveSO2ModelCenterPos(IRoot* lockobj = NULL);
	~EveSO2ModelCenterPos();

	void UpdateValue( double time ) { Vector3 v; Update( &v, time ); }

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriVectorFunction
	/////////////////////////////////////////////////////////////////////////////////////

	Vector3* Update( Vector3* in, Be::Time time );
	Vector3* Update( Vector3* in, double time );
	Vector3* GetValueAt( Vector3* in, Be::Time time );
	Vector3* GetValueAt( Vector3* in, double time );
	Vector3* GetValueDotAt( Vector3* in, Be::Time time );
	Vector3* GetValueDotAt( Vector3* in, double time );
	Vector3* GetValueDoubleDotAt( Vector3* in, Be::Time time );
	Vector3* GetValueDoubleDotAt( Vector3* in, double time );
	Vector3d* InterpolatedPosition(Vector3d* out, Be::Time time);


};
TYPEDEF_BLUECLASS(EveSO2ModelCenterPos);
#endif 