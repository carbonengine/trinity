/* 
	*************************************************************************

	ITriFunction.h

	Author:    Hilmar Veigar Pétursson
	Created:   August 2001
	OS:        Win32
	Project:   Trinity

	Description:   

		Yeap


	Dependencies:

		DirectX 9.0, Probably more, ytbd.

	(c) CCP 2000

	*************************************************************************
*/

#ifndef _ITRIFUNCTION_H_
#define _ITRIFUNCTION_H_

#include "ITriConstants.h"

struct Vector3d;		
struct Vector3;

// This interface is the basis of our curves
BLUE_INTERFACE(ITriFunction) : public IRoot
{
	virtual void UpdateValue( double time ) = 0;
	virtual void Reset() {}
};

BLUE_INTERFACE(ITriScalarFunction) : public ITriFunction
{
	// Get the value of the function based on Be::Time 	
	virtual float Update(
		Be::Time time
		) = 0;

	// Get the value of the function based on double time	
	virtual float Update(
		double time
		) = 0;

	// Get the value of the function based on Be::Time 
	// without affecting the internal state the the class
	virtual float GetValueAt(		
		Be::Time time
		) = 0;
	
	// Get the value of the function based on double time
	// without affecting the internal state the the class
	virtual float GetValueAt(
		double time
		) = 0;

	virtual void ScaleTime(
		float s
		) = 0;
};

BLUE_INTERFACE(ITriVectorFunction) : public ITriFunction
{
	// Get the value of the function based on Be::Time 
	// the pointer returned should be to a member of the class
	virtual Vector3* Update(
		Vector3* in,
		Be::Time time
		) = 0;

	// Get the value of the function based on double time
	// the pointer returned should be to a member of the class
	virtual Vector3* Update(
		Vector3* in,
		double time
		) = 0;

	// Get the value of the function based on Be::Time 
	// without affecting the internal state the the class
	virtual Vector3* GetValueAt(
		Vector3* in,
		Be::Time time
		) = 0;
	
	// Get the value of the function based on double time
	// without affecting the internal state the the class
	virtual Vector3* GetValueAt(
		Vector3* in,
		double time
		) = 0;

	// Get the value of the function based on Be::Time 
	// without affecting the internal state the the class
	virtual Vector3* GetValueDotAt(
		Vector3* in,
		Be::Time time
		) = 0;
	
	// Get the value of the function based on double time
	// without affecting the internal state the the class
	virtual Vector3* GetValueDotAt(
		Vector3* in,
		double time
		) = 0;

	// Get the value of the function based on Be::Time 
	// without affecting the internal state the the class
	virtual Vector3* GetValueDoubleDotAt(
		Vector3* in,
		Be::Time time
		) = 0;
	
	// Get the value of the function based on double time
	// without affecting the internal state the the class
	virtual Vector3* GetValueDoubleDotAt(
		Vector3* in,
		double time
		) = 0;

	virtual Vector3d* InterpolatedPosition(Vector3d* out, Be::Time time) = 0;
};


BLUE_INTERFACE(ITriQuaternionFunction) : public ITriFunction
{
	// Get the value of the function based on Be::Time 
	// the pointer returned should be to a member of the class
	virtual Quaternion* Update(
		Quaternion* in,
		Be::Time time
		) = 0;

	// Get the value of the function based on double time
	// the pointer returned should be to a member of the class
	virtual Quaternion* Update(
		Quaternion* in,
		double time
		) = 0;

	// Get the value of the function based on Be::Time 
	// without affecting the internal state the the class
	virtual Quaternion* GetValueAt(
		Quaternion* in,
		Be::Time time
		) = 0;
	
	// Get the value of the function based on double time
	// without affecting the internal state the the class
	virtual Quaternion* GetValueAt(
		Quaternion* in,
		double time
		) = 0;

	// Get the value of the function based on Be::Time 
	// without affecting the internal state the the class
	virtual Quaternion* GetValueDotAt(
		Quaternion* in,
		Be::Time time
		) = 0;
	
	// Get the value of the function based on double time
	// without affecting the internal state the the class
	virtual Quaternion* GetValueDotAt(
		Quaternion* in,
		double time
		) = 0;

	// Get the value of the function based on Be::Time 
	// without affecting the internal state the the class
	virtual Quaternion* GetValueDoubleDotAt(
		Quaternion* in,
		Be::Time time
		) = 0;
	
	// Get the value of the function based on double time
	// without affecting the internal state the the class
	virtual Quaternion* GetValueDoubleDotAt(
		Quaternion* in,
		double time
		) = 0;
};


BLUE_INTERFACE(ITriColorFunction) : public ITriFunction
{
	// Get the value of the function based on Be::Time 
	// the pointer returned should be to a member of the class
	virtual Color* Update(
		Color* in,
		Be::Time time
		) = 0;

	// Get the value of the function based on double time
	// the pointer returned should be to a member of the class
	virtual Color* Update(
		Color* in,
		double time
		) = 0;

	// Get the value of the function based on Be::Time 
	// without affecting the internal state the the class
	virtual Color* GetValueAt(
		Color* in,
		Be::Time time
		) = 0;
	
	// Get the value of the function based on double time
	// without affecting the internal state the the class
	virtual Color* GetValueAt(
		Color* in,
		double time
		) = 0;
};



#endif