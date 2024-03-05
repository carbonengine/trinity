/* 
	*************************************************************************

	ITriVector.h

	Author:    Hilmar Veigar Pétursson
	Created:   November 2000
	OS:        Win32
	Project:   Trinity

	Description:   

		Interface for accessing Vectors that are used in Trinity


	Dependencies:

		Probably, ytbd.

	(c) CCP 2000

	*************************************************************************
*/

#ifndef _ITRIVECTOR_H_
#define _ITRIVECTOR_H_

struct Vector3;
struct Quaternion;

BLUE_INTERFACE( ITriVector ) : public IRoot
{
	// Construction and data access
	virtual void SetXYZ(
		float x, 
		float y, 
		float z
		) = 0;

	virtual void SetVector(
		const Vector3* v
		) = 0;

	virtual const Vector3* GetVector(
		) const =0;

	virtual Vector3* CopyVector(
		Vector3* in
		) const =0;

	virtual Vector3* Vector(
		) = 0;

	// Math functions
	virtual float Length(
		) const = 0;

	virtual void Scale(
		float s
		) = 0;

	virtual void Normalize(
		) = 0;

	virtual void SetCrossProduct(
		const Vector3* v1,
		const Vector3* v2
		) = 0;

	virtual float DotProduct(
		const Vector3* v1
		) = 0;

	virtual void TransformQuaternion(
		const Quaternion* in
		) = 0;
};

#endif