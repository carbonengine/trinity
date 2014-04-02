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
	return PyString_FromString(buf);
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
	D3DXQuaternionIdentity(this);
}	


void TriQuaternion::SetRotationAxis(
	const Vector3* axis, 
	float angle
	)
{
	
	D3DXQuaternionRotationAxis(this, axis, angle);
}


void TriQuaternion::GetRotationAxis(
	Vector3* axis, 
	float* angle
	) const
{
	D3DXQuaternionToAxisAngle(this, axis, angle);

	// this would more safe but too many lines :-(
	//Vector3* t; 
	//D3DXQuaternionToAxisAngle(this, &t, angle);
	//(Vector3*)axis->Data();
}


void TriQuaternion::SetYawPitchRoll(
	float yaw, 
	float pitch, 
	float roll
	)
{
	D3DXQuaternionRotationYawPitchRoll(this, yaw, pitch, roll);
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
	
	D3DXQuaternionRotationYawPitchRoll(this, yawCurr, pitchCurr, rollCurr);
	return;

}


void TriQuaternion::IncreaseLocalYawPitchRoll(
	float yaw, 
	float pitch, 
	float roll
	)
{
	::Quaternion diff;
	D3DXQuaternionRotationYawPitchRoll(&diff, yaw, pitch, roll);

	D3DXQuaternionMultiply(this, &diff, this);
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
	D3DXQuaternionMultiply(this, in, this);
}

void TriQuaternion::SetSLERP(
	const ::Quaternion* q1,
	const ::Quaternion* q2,
	const float t
	)
{
	D3DXQuaternionSlerp(this, q1, q2, t);
}

void TriQuaternion::Normalize()
{
	D3DXQuaternionNormalize(this, this);
}

float TriQuaternion::Length() const
{
	return D3DXQuaternionLength(this);
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

void TriQuaternion::PyConjugate()
{	
	D3DXQuaternionConjugate(this, this);	
}

float TriQuaternion::PyDot( ITriQuaternion* other )
{
	return D3DXQuaternionDot( this, other->GetQuaternion() );
}


void TriQuaternion::PyExp()
{
	D3DXQuaternionExp(this, this);	
}


void TriQuaternion::PyIdentity()
{
	D3DXQuaternionIdentity( this );	
}

void TriQuaternion::PyInverse()
{
	D3DXQuaternionInverse( this, this );	
}


int TriQuaternion::PyIsIdentity()
{
	return int( D3DXQuaternionIsIdentity( this ) );	
}


float TriQuaternion::PyLength()
{
	return D3DXQuaternionLength( this );	
}


float TriQuaternion::PyLengthSq()
{
	return D3DXQuaternionLengthSq( this );	
}


void TriQuaternion::PyLn()
{
	D3DXQuaternionLn( this, this );	
}


void TriQuaternion::PyMultiply( ITriQuaternion* other )
{
	D3DXQuaternionMultiply( this, this, other->GetQuaternion() );
}


void TriQuaternion::PyNormalize()
{
	D3DXQuaternionNormalize( this, this );	
}

void TriQuaternion::PyRotationAxis( ITriVector* axis, float angle )
{		
	D3DXQuaternionRotationAxis( this, axis->GetVector(), angle );
}

void TriQuaternion::PyRotationMatrix( ITriMatrix* matrix )
{		
	D3DXQuaternionRotationMatrix( this, matrix->GetMatrix() );
}

void TriQuaternion::PyYawPitchRoll( float yaw, float pitch, float roll )
{		
	D3DXQuaternionRotationYawPitchRoll( this, yaw, pitch, roll );
}

void TriQuaternion::PySlerp( ITriQuaternion* other, float t )
{		
	D3DXQuaternionSlerp( this, this, other->GetQuaternion(), t );
}


PyObject* TriQuaternion::PyToAxisAngle(
	PyObject* args
	)
{		

	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	float angle;
	ITriVectorPtr v;
	v.Attach(new OTriVector);

	Vector3 dv;
	D3DXQuaternionToAxisAngle(this, &dv, &angle);
	v->SetVector(&dv);

	PyObject* v1 = PyOS->WrapBlueObject(v);
	PyObject* ret = Py_BuildValue("Of",v1,angle);
	Py_DECREF(v1);

	return ret;
}

void TriQuaternion::PySetRotationAxis( ITriVector* axis, float angle )
{		
	SetRotationAxis( axis->GetVector(), angle );
}


PyObject* TriQuaternion::PyGetRotationAxis(
	PyObject* args
	)
{		
	float angle;
	ITriVectorPtr v;
	v.Attach(new OTriVector);
	
	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	Vector3 dv;
	GetRotationAxis(&dv, &angle);
	v->SetVector(&dv);

	PyObject* v1 = PyOS->WrapBlueObject(v);
	PyObject* ret = Py_BuildValue("Of",v1,angle);
	Py_DECREF(v1);

	return ret;
}

Vector3 TriQuaternion::PyGetYawPitchRoll()
{		
	Vector3 ypr;
	GetYawPitchRoll( &ypr.x, &ypr.y, &ypr.z );
	return ypr;
	
}

void TriQuaternion::PySetRotationArc( ITriVector* v1, ITriVector* v2 )
{		
	SetRotationArc( v1->GetVector(), v2->GetVector() );
}

void TriQuaternion::PyMultiplyQuaternion( ITriQuaternion* other )
{	
	MultiplyQuaternion( other->GetQuaternion() );
}

void TriQuaternion::PySetSLERP( ITriQuaternion* q1, ITriQuaternion* q2, float t )
{
	SetSLERP( q1->GetQuaternion(), q2->GetQuaternion(), t );
}

void TriQuaternion::PyScale( float factor )
{	
	TriQuaternionScale( this, this, factor );
}

void TriQuaternion::PySetExp( ITriQuaternion* other )
{
	D3DXQuaternionExp( this, other->GetQuaternion() );
}

void TriQuaternion::PySetLn( ITriQuaternion* other )
{
	D3DXQuaternionLn( this, other->GetQuaternion() );
}

void TriQuaternion::PyPow( float power )
{
	TriQuaternionPow( this, this, power );
}






#endif
