/* 
	*************************************************************************************

	TriScalarKey.h

	Author:    Hilmar Veigar Pétursson
	Created:   November 2000
	OS:        Win32
	Project:   Trinity

	Description:   

		TriScalarKey defines keyframes in a scalar curve and a method to intepolate to
		another key.


	Dependencies:

		None

	(c) CCP 2000

	*************************************************************************************
*/

#ifndef _TRISCALARKEY_H_
#define _TRISCALARKEY_H_


#include "include/ITriScalarCurve.h"
#include "TriConstants.h"
#include "include/TriQuaternion.h"
#include "include/TriColor.h"


////////////////////////////////////////////////////////////////////////////
// TriScalarKey
////////////////////////////////////////////////////////////////////////////

BLUE_CLASS( TriScalarKey ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	float mTime;
	float mValue;
	float mLeft;
	float mRight;
	TRIINTERPOLATION mInterpolation;

	TriScalarKey(IRoot* lockobj = NULL);
};
TYPEDEF_BLUECLASS(TriScalarKey);

float Interpolate(
	const TriScalarKey* k1, 
	const TriScalarKey* k2, 
	float t
	);


////////////////////////////////////////////////////////////////////////////
// TriVectorKey
////////////////////////////////////////////////////////////////////////////

BLUE_CLASS( TriVectorKey ) : public	IRoot
{
public:	
	EXPOSE_TO_BLUE();

	float mTime;
	Vector3 mValue;
	Vector3 mLeft;
	Vector3 mRight;
	TRIINTERPOLATION mInterpolation;
	
	TriVectorKey(IRoot* lockobj = NULL);
};
TYPEDEF_BLUECLASS(TriVectorKey);

Vector3* Interpolate(
	Vector3* out,
	const TriVectorKey* k1, 
	const TriVectorKey* k2, 
	float t
	);


////////////////////////////////////////////////////////////////////////////
// TriQuaternionKey
////////////////////////////////////////////////////////////////////////////

BLUE_CLASS( TriQuaternionKey ) : public IRoot
{
public:	
	EXPOSE_TO_BLUE();

	float mTime;
	Quaternion mValue;
	Quaternion mLeft;
	Quaternion mRight;
	TRIINTERPOLATION mInterpolation;
	
	TriQuaternionKey(IRoot* lockobj = NULL);
};
TYPEDEF_BLUECLASS(TriQuaternionKey);

Quaternion* Interpolate(
	Quaternion* out,
	const TriQuaternionKey* k1, 
	const TriQuaternionKey* k2, 
	const TriQuaternionKey* k3, 
	const TriQuaternionKey* k4, 
	float t
	);


////////////////////////////////////////////////////////////////////////////
// TriColorKey
////////////////////////////////////////////////////////////////////////////

BLUE_CLASS( TriColorKey ) : public IRoot
{
public:	
	EXPOSE_TO_BLUE();

	float mTime;
	Color mValue;
	Color mLeft;
	Color mRight;
	TRIINTERPOLATION mInterpolation;
	
	TriColorKey(IRoot* lockobj = NULL);
};
TYPEDEF_BLUECLASS(TriColorKey);

Color* Interpolate(
	Color* out,
	const TriColorKey* k1, 
	const TriColorKey* k2, 
	float t
	);

#endif
