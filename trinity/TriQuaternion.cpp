#include "StdAfx.h"
#include "include/TriQuaternion.h"
#include "include/TriVector.h"
#include "include/TriMatrix.h"
#include "include/TriMath.h"


/////////////////////////////////////////////////////////////////////////////////////////
// IPythonMethods Impl
/////////////////////////////////////////////////////////////////////////////////////////

#if BLUE_WITH_PYTHON

void TriQuaternion::Destroy(
	)
{
	//delete this;
}


PyObject* TriQuaternion::GetAttr( 
	const char* name, 
	bool* handled
	)
{
	return 0;
}


bool TriQuaternion::SetAttr(
	const char* name, 
	PyObject* v, 
	bool* handled 
	)
{
	return true;
}


PyObject* TriQuaternion::Repr(
	bool* handled
	)
{
	*handled = true;
	char buf[120];
	sprintf_s(buf, "(%f,%f,%f,%f)", x, y, z,w);
	return PyUnicode_FromString(buf);
}


/////////////////////////////////////////////////////////////////////////////////////////
// TriQuaternion
/////////////////////////////////////////////////////////////////////////////////////////

TriQuaternion::TriQuaternion(IRoot* lockobj) :
::Quaternion(0.0f, 0.0f, 0.0f, 1.0f)
{
}


TriQuaternion::~TriQuaternion()
{
}


/////////////////////////////////////////////////////////////////////////////////////////
// ITriQuaternion Impl
/////////////////////////////////////////////////////////////////////////////////////////

void TriQuaternion::SetXYZW(
	float _x, 
	float _y, 
	float _z, 
	float _w
	)
{
	x = _x;
	y = _y;
	z = _z;
	w = _w;
}


void TriQuaternion::SetQuaternion(
	const ::Quaternion* ar
	)
{
	x = ar->x;
	y = ar->y;
	z = ar->z;
	w = ar->w;
}


const ::Quaternion* TriQuaternion::GetQuaternion(
	) const
{
	return this;    
}


::Quaternion* TriQuaternion::CopyQuaternion(
	::Quaternion* in
	) const
{
	// would this work?
	//return &(*in = *this);
	
	*in = *this;
	return in;
}


::Quaternion* TriQuaternion::Quaternion(
	)
{
	return this;
}


void TriQuaternion::SetIdentity()
{
	*static_cast<::Quaternion*>( this ) = IdentityQuaternion();
}	


void TriQuaternion::SetRotationAxis(
	const Vector3* axis, 
	float angle
	)
{
	*static_cast<::Quaternion*>( this ) = RotationQuaternion( *axis, angle );
}


void TriQuaternion::GetRotationAxis(
	Vector3* axis, 
	float* angle
	) const
{
	auto result = GetAxisAngle( *this );
	*axis = result.first;
	*angle = result.second;
}


void TriQuaternion::SetYawPitchRoll(
	float yaw, 
	float pitch, 
	float roll
	)
{
	*static_cast<::Quaternion*>( this ) = RotationQuaternion( yaw, pitch, roll );
}


void TriQuaternion::GetYawPitchRoll (
    float* yaw, 
    float* pitch, 
    float* roll
	) const
{
	TriQuaternionToYawPitchRoll( yaw, pitch, roll, this);
}
 

void TriQuaternion::IncreaseYawPitchRoll(
	float yaw, 
	float pitch, 
	float roll
	)
{
	float yawCurr; 
	float pitchCurr; 
	float rollCurr;

	TriQuaternionToYawPitchRoll(&yawCurr, &pitchCurr, &rollCurr, this);
	
	yawCurr += yaw;
	pitchCurr -= pitch;
	if(pitchCurr < -1.5f)
		pitchCurr = -1.5f;
	if(pitchCurr > 1.5f)
		pitchCurr = 1.5f;
	rollCurr += roll;
	
	*static_cast<::Quaternion*>( this ) = RotationQuaternion( yawCurr, pitchCurr, rollCurr );
	return;

}


void TriQuaternion::IncreaseLocalYawPitchRoll(
	float yaw, 
	float pitch, 
	float roll
	)
{
	::Quaternion diff = RotationQuaternion( yaw, pitch, roll );
	*static_cast<::Quaternion*>( this ) = diff * *this;
}


void TriQuaternion::SetRotationArc(
	const Vector3* v1, 
	const Vector3* v2
	)
{
	TriQuaternionRotationArc(this, v1, v2);
}


void TriQuaternion::MultiplyQuaternion(
	const ::Quaternion* in
	)
{
	*static_cast<::Quaternion*>( this ) = *in * *this;
}

void TriQuaternion::SetSLERP(
	const ::Quaternion* q1,
	const ::Quaternion* q2,
	const float t
	)
{
	*static_cast<::Quaternion*>( this ) = Slerp( *q1, *q2, t );
}

void TriQuaternion::Normalize()
{
	*static_cast<::Quaternion*>( this ) = ::Normalize( *this );
}

float TriQuaternion::Length() const
{
	return ::Length( *this );
}



/////////////////////////////////////////////////////////////////////////////////////////
// Python thunkers for ITriQuaternion interface
/////////////////////////////////////////////////////////////////////////////////////////

void TriQuaternion::Py__init__( float _x, float _y, float _z, Be::Optional<float> _w )
{
	x = _x;
	y = _y;
	z = _z;
	w = _w.IsAssigned() ? float( _w ) : 1.0f;
}

void TriQuaternion::PyIdentity()
{
	*static_cast<::Quaternion*>( this ) = IdentityQuaternion();
}

void TriQuaternion::PyInverse()
{
	*static_cast<::Quaternion*>( this ) = Inverse( *this );
}


float TriQuaternion::PyLength()
{
	return ::Length( *this );	
}


void TriQuaternion::PyMultiply( ITriQuaternion* other )
{
	*static_cast<::Quaternion*>( this ) = *this * *other->GetQuaternion();
}


void TriQuaternion::PyNormalize()
{
	*static_cast<::Quaternion*>( this ) = ::Normalize( *this );
}

void TriQuaternion::PyRotationAxis( ITriVector* axis, float angle )
{		
	*static_cast<::Quaternion*>( this ) = RotationQuaternion( *axis->GetVector(), angle );
}

void TriQuaternion::PySetRotationAxis( ITriVector* axis, float angle )
{		
	SetRotationAxis( axis->GetVector(), angle );
}

Vector3 TriQuaternion::PyGetYawPitchRoll()
{		
	Vector3 ypr;
	GetYawPitchRoll( &ypr.x, &ypr.y, &ypr.z );
	return ypr;
	
}

void TriQuaternion::PyScale( float factor )
{	
	TriQuaternionScale( this, this, factor );
}






#endif
