/* 
	*************************************************************************************

	TriRigidOrientation.h

	Created:   October 2001
	OS:        Win32
	Project:   Trinity

	Description:   

		See TRIRIGIDORIENTATION_Description



	Dependencies:

		None

	(c) CCP 2000

	*************************************************************************************
*/

#ifndef _TRIRIGIDORIENTATION_H_
#define _TRIRIGIDORIENTATION_H_

#define TRIRIGIDORIENTATION_Description \
"To be written"

#include "include/ITriFunction.h"

class TriTorque:
	public IRoot
{
public:	
	EXPOSE_TO_BLUE();

	float mTime;
	Quaternion mRot0;
	Vector3 mOmega0;
	Vector3 mTorque;		
	
	TriTorque(IRoot* lockobj = NULL);

public:
};
TYPEDEF_BLUECLASS(TriTorque);

BLUE_DECLARE_VECTOR( TriTorque );

class TriRigidOrientation:
	public ITriQuaternionFunction
{
public:					
	EXPOSE_TO_BLUE();

	std::wstring  mName;
	Be::Time mStart;
	float mI;
	float mDrag;
	Quaternion mValue;

	PTriTorqueVector mStates;
	int mCurrKey;
		
	/////////////////////////////////////////////////////////////////////////////////////
	// ITriFunction
	/////////////////////////////////////////////////////////////////////////////////////
	void UpdateValue( double time ) { Quaternion q; Update( &q, time ); }

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriVectorFunction
	/////////////////////////////////////////////////////////////////////////////////////
		
	Quaternion* Update(
		Quaternion* in,
		Be::Time time
		);

	Quaternion* Update(
		Quaternion* in,
		double time
		);
	
	Quaternion* GetValueAt(
		Quaternion* in,
		Be::Time time
		);

	Quaternion* GetValueAt(
		Quaternion* in,
		double time
		);

	Quaternion* GetValueDotAt(
		Quaternion* in,
		Be::Time time
		);

	Quaternion* GetValueDotAt(
		Quaternion* in,
		double time
		);

	Quaternion* GetValueDoubleDotAt(
		Quaternion* in,
		Be::Time time
		);

	Quaternion* GetValueDoubleDotAt(
		Quaternion* in,
		double time
		);

	/////////////////////////////////////////////////////////////////////////////////////
	// Other :-)
	/////////////////////////////////////////////////////////////////////////////////////
		
	Vector3* GetValueDotAt(
		Vector3* in,
		Be::Time time
		);

	Vector3* GetValueDotAt(
		Vector3* in,
		double time
		);

	void Sort(
		);

	TriRigidOrientation(IRoot* lockobj = NULL);
	~TriRigidOrientation();
private:
	Vector3 mTauVector;
	Quaternion mTauConverter;
	void Seek(
		double t
		);	
};
TYPEDEF_BLUECLASS(TriRigidOrientation);

#endif