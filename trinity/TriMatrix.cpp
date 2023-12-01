#include "StdAfx.h"
#include "include/TriMatrix.h"
#include "include/TriVector.h"
#include "include/TriQuaternion.h"


#if BLUE_WITH_PYTHON
/////////////////////////////////////////////////////////////////////////////////////////
// IPythonMethods Impl
/////////////////////////////////////////////////////////////////////////////////////////

void TriMatrix::Destroy(
	)
{
	//delete this;
}


PyObject* TriMatrix::GetAttr( 
	const char* name, 
	bool* handled
	)
{
	return 0;
}


bool TriMatrix::SetAttr(
	const char* name, 
	PyObject* v, 
	bool* handled 
	)
{
	return true;
}


PyObject* TriMatrix::Repr(
	bool* handled
	)
{
	*handled = true;
	char buf[256];
	sprintf_s(buf, "<Instance of 'TriMatrix'\r\n"
		"( %0.1f %0.1f %0.1f %0.1f\r\n"
		"  %0.1f %0.1f %0.1f %0.1f\r\n"
		"  %0.1f %0.1f %0.1f %0.1f\r\n"
		"  %0.1f %0.1f %0.1f %0.1f )>", 
		_11, _12, _13, _14,
		_21, _22, _23, _24,
		_31, _32, _33, _34,
		_41, _42, _43, _44		
		);
	return PyString_FromString(buf);
}
#endif


/////////////////////////////////////////////////////////////////////////////////////////
// TriMatrix
/////////////////////////////////////////////////////////////////////////////////////////

TriMatrix::TriMatrix(IRoot* lockobj) :
	Matrix(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f)
{
}


TriMatrix::~TriMatrix()
{
}


/////////////////////////////////////////////////////////////////////////////////////////
// ITriMatrix Impl
/////////////////////////////////////////////////////////////////////////////////////////

void TriMatrix::SetPivots(
	float _11_, float _12_, float _13_, float _14_,
	float _21_, float _22_, float _23_, float _24_,
	float _31_, float _32_, float _33_, float _34_,
	float _41_, float _42_, float _43_, float _44_
	)
{
	_11 = _11_; _12 = _12_; _13 = _13_; _14 = _14_;
	_21 = _21_; _22 = _22_; _23 = _23_; _24 = _24_;
	_31 = _31_; _32 = _32_; _33 = _33_; _34 = _34_;
	_41 = _41_; _42 = _42_; _43 = _43_; _44 = _44_;
}

void TriMatrix::SetMatrix(
	const Matrix* m
	)
{
	_11 = m->_11; _12 = m->_12; _13 = m->_13; _14 = m->_14;
	_21 = m->_21; _22 = m->_22; _23 = m->_23; _24 = m->_24;
	_31 = m->_31; _32 = m->_32; _33 = m->_33; _34 = m->_34;
	_41 = m->_41; _42 = m->_42; _43 = m->_43; _44 = m->_44;

}

const Matrix* TriMatrix::GetMatrix(
	) const
{
	return this;
}

Matrix* TriMatrix::CopyMatrix(
	Matrix* in
	) const
{
	*in = *this;
	return in;
}

float TriMatrix::Determinant(	  
	) 
{
	return ::Determinant( *this );
}

void TriMatrix::Identity(
)
{
	static_cast<Matrix&>( *this ) = IdentityMatrix();
}

float TriMatrix::Inverse(
	) 
{
	float ret;
	::Inverse( *this, ret, Matrix( *this ) );
	return ret;
}

void TriMatrix::LookAtRH(	  
	const Vector3* pEye,
	const Vector3* pAt,
	const Vector3* pUp
	) 
{
	*static_cast<Matrix*>( this ) = LookAtMatrix( *pEye, *pAt, *pUp );
}

void TriMatrix::Multiply(		  
	const Matrix* pM2
	) 
{
	*static_cast<Matrix*>( this ) = *this * *pM2;
}

void TriMatrix::RotationAxis(	  
	const Vector3* pV,
	float Angle
	) 
{
	*static_cast<Matrix*>( this ) = RotationMatrix( *pV, Angle );
}

void TriMatrix::RotationQuaternion(	
	const Quaternion* pQ
	) 
{
	*static_cast<Matrix*>( this ) = RotationMatrix( *pQ );
}

void TriMatrix::RotationX(	  
	float Angle
	) 
{
	*static_cast<Matrix*>( this ) = RotationXMatrix( Angle );
}

void TriMatrix::RotationY(
	float Angle
	) 
{
	*static_cast<Matrix*>( this ) = RotationYMatrix( Angle );
}

void TriMatrix::RotationYawPitchRoll(
	float Yaw,
	float Pitch,
	float Roll
	) 
{
	*static_cast<Matrix*>( this ) = RotationMatrix( ::RotationQuaternion( Yaw, Pitch, Roll ) );
}

void TriMatrix::RotationZ(
	float Angle
	) 
{
	*static_cast<Matrix*>( this ) = RotationZMatrix( Angle );
}

void TriMatrix::Scaling(	  
	float sx,
	float sy,
	float sz
	) 
{
	*static_cast<Matrix*>( this ) = ScalingMatrix( sx, sy, sz );
}

void TriMatrix::Transformation(	  
	const Vector3* pScalingCenter,
	const Quaternion* pScalingRotation,
	const Vector3* pScaling,
	const Vector3* pRotationCenter,
	const Quaternion* pRotation,
	const Vector3* pTranslation
	) 
{
	*static_cast<Matrix*>( this ) = TransformationMatrix(
		pScalingCenter,
		pScalingRotation,
		pScaling,
		pRotationCenter,
		pRotation,
		pTranslation);
}

void TriMatrix::Translation(	  
	float x,
	float y,
	float z
	) 
{
	*static_cast<Matrix*>( this ) = TranslationMatrix( x, y, z );
}

void TriMatrix::Transpose(	  	  
	)
{
	*static_cast<Matrix*>( this ) = ::Transpose( *this );
}


#if BLUE_WITH_PYTHON
/////////////////////////////////////////////////////////////////////////////////////////
// Python thunkers for ITriMatrix interface
/////////////////////////////////////////////////////////////////////////////////////////

PyObject* TriMatrix::Py__init__(
	PyObject* args
	)
{		
	if (!PyArg_ParseTuple(args, "|ffffffffffffffff", 
		&_11, &_12, &_13, &_14,
		&_21, &_22, &_23, &_24,
		&_31, &_32, &_33, &_34,
		&_41, &_42, &_43, &_44))
		return NULL;	
	
	Py_INCREF(Py_None);
	return Py_None;
}
#endif


void TriMatrix::PyLookAtRH( ITriVector* eye, ITriVector* at, ITriVector* up ) 
{
	LookAtRH(
		eye->GetVector(),
		at->GetVector(),
		up->GetVector() );
}

void TriMatrix::PyMultiply( ITriMatrix* other )
{
	Multiply( other->GetMatrix() );
}

void TriMatrix::PyRotationAxis( ITriVector* axis, float angle )
{
	RotationAxis( axis->GetVector(), angle );
}

void TriMatrix::PyRotationQuaternion( ITriQuaternion* quaternion )
{
	RotationQuaternion( quaternion->GetQuaternion() );
}

void TriMatrix::PyTransformation( 
	ITriVector* scalingCenter,
	ITriQuaternion* scalingRotation,
	ITriVector* scaling,
	ITriVector* rotationCenter,
	ITriQuaternion* rotation,
	ITriVector* translation )
{
	Transformation(
		scalingCenter->GetVector(),
		scalingRotation->GetQuaternion(),
		scaling->GetVector(),
		rotationCenter->GetVector(),
		rotation->GetQuaternion(),
		translation->GetVector() );		
}
