/* 
	*************************************************************************************

	TriMatrix.h

	Author:    Hilmar Veigar Pétursson
	Created:   September 2001
	OS:        Win32
	Project:   Trinity

	Description:   

		See TRIMATRIX_Description


	Dependencies:

		DirectX 9.0, Blue

	(c) CCP 2000

	*************************************************************************************
*/


#ifndef _TRIMATRIX_H_
#define _TRIMATRIX_H_

#define TRIMATRIX_Description \
"Simple mapping of Matrix over to Blue so that it can be used in Python"

#include "ITriMatrix.h"

#pragma warning (disable: 4275) // non dll-interface struct 'Matrix'

BLUE_INTERFACE( ITriVector );
BLUE_INTERFACE( ITriQuaternion );

class TriMatrix :
	public ITriMatrix,
#if BLUE_WITH_PYTHON
	public IPythonMethods,
#endif
	public Matrix
{
public:
	EXPOSE_TO_BLUE();

	TriMatrix(IRoot* lockobj = NULL);
	~TriMatrix();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriMatrix
	/////////////////////////////////////////////////////////////////////////////////////
	void SetPivots(
		float _11, float _12, float _13, float _14,
		float _21, float _22, float _23, float _24,
		float _31, float _32, float _33, float _34,
		float _41, float _42, float _43, float _44
		);

	void SetMatrix(
		const Matrix* m
		);

	const Matrix* GetMatrix(
		) const;

	Matrix* CopyMatrix(
		Matrix* in
		) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// IMatrix
	/////////////////////////////////////////////////////////////////////////////////////

	float Determinant(	  
		); 

	void Identity(
		); 

	float Inverse(
		); 

	void LookAtRH(	  
		const Vector3* pEye,
		const Vector3* pAt,
		const Vector3* pUp
		); 

	void Multiply(		  
	  const Matrix* pM2
	); 

	void RotationAxis(	  
		const Vector3* pV,
		float Angle
		); 

	void RotationQuaternion(	
		const Quaternion* pQ
		); 

	void RotationX(	  
		float Angle
		); 

	void RotationY(
		float Angle
		); 
	
	void RotationYawPitchRoll(
		float Yaw,
		float Pitch,
		float Roll
		); 

	void RotationZ(
		float Angle
		); 

	void Scaling(	  
		float sx,
		float sy,
		float sz
		); 

	void Transformation(	  
		const Vector3* pScalingCenter,
		const Quaternion* pScalingRotation,
		const Vector3* pScaling,
		const Vector3* pRotationCenter,
		const Quaternion* pRotation,
		const Vector3* pTranslation
		); 

	void Translation(	  
		float x,
		float y,
		float z
		); 

	void Transpose(	  	  
		); 



#if BLUE_WITH_PYTHON
	/////////////////////////////////////////////////////////////////////////////////////
	// IPythonMethods
	/////////////////////////////////////////////////////////////////////////////////////
	void Destroy(
		);

	PyObject* GetAttr( 
		const char* name, 
		bool* handled
		);

	bool SetAttr(
		const char* name,
		PyObject* v,
		bool* handled
		);

	PyObject* Repr(
		bool* handled
		);
#endif

public:
#if BLUE_WITH_PYTHON
	PyObject* Py__init__( PyObject* args );
#endif
	void PyLookAtRH( ITriVector* eye, ITriVector* at, ITriVector* up );
	void PyMultiply( ITriMatrix* other );
	void PyRotationAxis( ITriVector* axis, float angle );
	void PyRotationQuaternion( ITriQuaternion* quaternion );
	void PyTransformation( 
		ITriVector* scalingCenter,
		ITriQuaternion* scalingRotation,
		ITriVector* scaling,
		ITriVector* rotationCenter,
		ITriQuaternion* rotation,
		ITriVector* translation );
};
TYPEDEF_BLUECLASS(TriMatrix);

#endif

