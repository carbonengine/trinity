#include "StdAfx.h"
#include "include/TriVector.h"
#include "include/TriQuaternion.h"
#include "include/TriMatrix.h"
#include "Include/TriMath.h"
#include "TriViewport.h"

void GetNearestPointOnAABB(Vector3 &out, const Vector3 &p, const Vector3 &min, const Vector3 &max)
{
    if(p.x < min.x)
    {
        out.x = min.x;
    }
    else if(p.x > max.x)
    {
        out.x = max.x;
    }
    else
    {
        out.x = p.x;
    }

    if(p.y < min.y)
    {
        out.y = min.y;
    }
    else if(p.y > max.y)
    {
        out.y = max.y;
    }
    else
    {
        out.y = p.y;
    }

    if(p.z < min.z)
    {
        out.z = min.z;
    }
    else if(p.z > max.z)
    {
        out.z = max.z;
    }
    else
    {
        out.z = p.z;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
// IPythonMethods Impl
/////////////////////////////////////////////////////////////////////////////////////////

#if BLUE_WITH_PYTHON
void TriVector::Destroy()
{
	//delete this;
}


PyObject* TriVector::GetAttr( 
	const char* name, 
	bool* handled
	)
{
	return 0;
}


bool TriVector::SetAttr(
	const char* name, 
	PyObject* v, 
	bool* handled 
	)
{
	return true;
}


PyObject* TriVector::Repr(
	bool* handled
	)
{
	*handled = true;
	char buf[120];
	int size = sprintf_s(buf, "(%.3f, %.3f, %.3f)", x, y, z);
	return PyString_FromStringAndSize(buf, size);
}


/////////////////////////////////////////////////////////////////////////////////////////
// IPythonMethods Impl
/////////////////////////////////////////////////////////////////////////////////////////

bool TriVector::BinaryOp(
	PYNUMERIC_OPS op,
	IRoot* other,
	PyObject** retval
	)
{
	// The return value
	TriVectorPtr retvec;
	retvec.CreateInstance();
	retvec->SetVector( GetVector() );

	// This cast is safe, because 'other' is ALWAYS of
	// the same type as 'this'
	TriVector* vec2 = (TriVector*)(ITriVector*)other;


	// Due to the dubious type coercion where a float value
	// is converted to TriVector, vec2 from 'other' can be
	// an instance from such conversion.

	switch(op)
	{
	case PYOP_ADD:
		*retvec += *vec2;
		break;

	case PYOP_SUB:
		*retvec -= *vec2;
		break;

	case PYOP_MUL:
		*retvec *= vec2->x;
		break;

	case PYOP_DIV:
		*retvec /= vec2->x;
		break;

	default:
		return false;
	}

	*retval = PyOS->WrapBlueObject(retvec->GetRawRoot());
	return true;
}


bool TriVector::UnaryOp(
	PYNUMERIC_OPS op,
	PyObject** retval
	)
{
	// The return value
	ITriVectorPtr retvec;
	retvec.Attach(new OTriVector);	

	switch (op)
	{
	case PYOP_NEG:
		{
			Vector3 tmp = -(*this);
			retvec->SetVector( &tmp );
		}
		break;

	case PYOP_POS:
		{
			Vector3 tmp = +(*this);
			retvec->SetVector( &tmp );
		}
		break;

	default:
		// free the 'retvec'
		return false;
	}
	*retval = PyOS->WrapBlueObject(retvec);
	return true;
}


void TriVector::Coercion(
	PyObject* from,
	PyObject** to
	)
{
	// check for None first - never coerce this type! <halldor 2006.07.13>
	if( from == Py_None )
	{
		return;
	}

	// convert from any standard numeric value
	PyObject* pyfloat = PyNumber_Float(from);

	if (!pyfloat)
		return;

	float f = (float)PyFloat_AS_DOUBLE(pyfloat);
	Py_DECREF(pyfloat);

	// the return value
	ITriVectorPtr vec;
	vec.Attach(new OTriVector);	
	vec->SetXYZ(f, f, f);
	*to = PyOS->WrapBlueObject(vec);
}
#endif


/////////////////////////////////////////////////////////////////////////////////////////
// TriVector
/////////////////////////////////////////////////////////////////////////////////////////

TriVector::TriVector(IRoot* lockobj) :
	Vector3(0.0f, 0.0f,0.0f)
{
}


TriVector::~TriVector()
{
}


/////////////////////////////////////////////////////////////////////////////////////////
// ITriVector Impl
/////////////////////////////////////////////////////////////////////////////////////////

void TriVector::SetXYZ(
	float _x, 
	float _y, 
	float _z
	)
{
	x = _x;
	y = _y;
	z = _z;
}


void TriVector::SetVector(
	const Vector3* ar
	)
{
	x = ar->x;
	y = ar->y;
	z = ar->z;
}


const Vector3* TriVector::GetVector(
	) const
{
	return this;    
}

Vector3* TriVector::CopyVector(
	Vector3* in
	) const
{
	// would this work?
	//return &(*in = *this);
	
	*in = *this;
	return in;
}


Vector3* TriVector::Vector(
	)
{
	return this;
}


void TriVector::SetCrossProduct(
	const Vector3* v1, 
	const Vector3* v2
	)
{
	
	D3DXVec3Cross(this, v1, v2);
}


float TriVector::Length(
	) const
{
	return D3DXVec3Length(this);
}


float TriVector::LengthSq(
	) const
{
	return D3DXVec3LengthSq(this);
}


void TriVector::Scale(
	float s
	)
{
	D3DXVec3Scale(this, this, s);
}


void TriVector::Normalize(
	)
{
	D3DXVec3Normalize(this, this);
}


void TriVector::TransformQuaternion(
	const Quaternion* in
	)
{
	TriVectorRotateQuaternion(this, this, in);
}


float TriVector::DotProduct(
	const Vector3* v2
	)
{
	return D3DXVec3Dot(this, v2);
}

void TriVector::PyAdd( ITriVector* other )
{
	D3DXVec3Add( this, this, other->GetVector() );	
}

void TriVector::PyCross( ITriVector* other )
{
	D3DXVec3Cross( this, this, other->GetVector() );	
}

float TriVector::PyDot( ITriVector* other )
{
	return D3DXVec3Dot( this, other->GetVector() );
}

void TriVector::PyLerp( ITriVector* other, float t )
{
	D3DXVec3Lerp( this, this, other->GetVector(), t );	
}

void TriVector::PyMaximize( ITriVector* other )
{
	D3DXVec3Maximize( this, this, other->GetVector() );	
}

void TriVector::PyMinimize( ITriVector* other )
{
	D3DXVec3Minimize( this, this, other->GetVector() );	
}

void TriVector::PyProject(
	TriViewport* vp,
	ITriMatrix* project,
	ITriMatrix* view,
	ITriMatrix* world )
{
	D3DXVec3TransformCoord( this, this, world->GetMatrix() );
	D3DXVec3TransformCoord( this, this, view->GetMatrix() );
	D3DXVec3TransformCoord( this, this, project->GetMatrix() );
	Vec3TransformByViewport( *this, *vp );
}

void TriVector::PySubtract( ITriVector* other )
{
	D3DXVec3Subtract( this, this, other->GetVector() );	
}

void TriVector::PyTransformCoord( ITriMatrix* transform )
{
	D3DXVec3TransformCoord( this, this, transform->GetMatrix() );	
}

void TriVector::PyTransformNormal( ITriMatrix* transform )
{
	D3DXVec3TransformNormal( this, this, transform->GetMatrix() );	
}

void TriVector::PyUnproject(
	TriViewport* vp,
	ITriMatrix* project,
	ITriMatrix* view,
	ITriMatrix* world )
{
    Vector3 preViewport;
    x = 2.0f * ( x - vp->x ) / vp->width - 1.0f;
    y = 1.0f - 2.0f * ( y - vp->y ) / vp->height;
    z = ( z - vp->minZ ) / ( vp->maxZ - vp->minZ );
    
    Matrix worldViewProjInv;
    D3DXMatrixMultiply( &worldViewProjInv, world->GetMatrix(), view->GetMatrix() );
    D3DXMatrixMultiply( &worldViewProjInv, &worldViewProjInv, project->GetMatrix() );
    D3DXMatrixInverse( &worldViewProjInv, nullptr, &worldViewProjInv );
    
    D3DXVec3TransformCoord( this, this, &worldViewProjInv );
}

void TriVector::PyTransformQuaternion( ITriQuaternion* rotation )
{
	TransformQuaternion( rotation->GetQuaternion() );
}

void TriVector::PySetCrossProduct( ITriVector* v1, ITriVector* v2 )
{
	SetCrossProduct( v1->GetVector(), v2->GetVector() );
}

float TriVector::PyDotProduct( ITriVector* other )
{
	return DotProduct( other->GetVector() );
}
