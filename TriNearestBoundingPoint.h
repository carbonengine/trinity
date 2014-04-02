/* 
	*************************************************************************************

	TriNearestBoundingPoint.h

	Created:   August 2005
	OS:        Win32
	Project:   Trinity

	Description:   

		TriNearestBoundingPoint is a ITriVectorFunction compatible object that calculates the 
		point on the bounding sphere of a transform nearest to another transform.
		It will mostly be used for effects that need to stretch to the 'shield' edge of an object.

		NB: Much like the 

	(c) CCP 2005

	*************************************************************************************
*/

#ifndef _TRINEARESTBOUNDINGPOINT_H_
#define _TRINEARESTBOUNDINGPOINT_H_

#define TRINEARESTBOUNDINGPOINT_Description "TriNearestBoundingPoint"

/* is a ITriVectorFunction compatible object that calculates the\r\n 
point on the bounding sphere of a transform nearest to another transform.\r\n
It will mostly be used for effects that need to stretch to the 'shield' edge of an object." */

#include "include/ITriFunction.h"

class TriNearestBoundingPoint:
	public ITriVectorFunction
{

public:
	EXPOSE_TO_BLUE();

	TriNearestBoundingPoint(IRoot* lockobj = NULL);
	~TriNearestBoundingPoint();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriVectorFunction
	/////////////////////////////////////////////////////////////////////////////////////

	void UpdateValue( double time ) { Vector3 v; Update( &v, time ); }

	Vector3* Update(
		Vector3* in,
		Be::Time time
		);

	Vector3* Update(
		Vector3* in,
		double time
		);

	Vector3* GetValueAt(
		Vector3* in,
		Be::Time time
		);
	
	Vector3* GetValueAt(
		Vector3* in,
		double time
		);

	Vector3* GetValueDotAt(
		Vector3* in,
		Be::Time time
		);
	
	Vector3* GetValueDotAt(
		Vector3* in,
		double time
		);

	Vector3* GetValueDoubleDotAt(
		Vector3* in,
		Be::Time time
		);
	
	Vector3* GetValueDoubleDotAt(
		Vector3* in,
		double time
		);

	Vector3d* InterpolatedPosition(Vector3d* out, Be::Time time);

private:
	ITriVectorFunctionPtr		m_parentPositionCurve;
	ITriQuaternionFunctionPtr	m_parentRotationCurve;
	ITriVectorFunctionPtr		m_alignPositionCurve;
	Vector3 m_value;
	Vector3 m_boundingBoxSize;

public:
};
TYPEDEF_BLUECLASS(TriNearestBoundingPoint);
#endif 
