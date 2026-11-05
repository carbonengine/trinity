// Copyright © 2000 CCP ehf.

#ifndef _TRIVECTOR_H_
#define _TRIVECTOR_H_

#include "ITriVector.h"

#pragma warning( disable : 4275 ) // non dll-interface struct 'Vector3'

BLUE_DECLARE( TriVector );
BLUE_DECLARE( TriViewport );
BLUE_DECLARE_INTERFACE( ITriMatrix );
BLUE_DECLARE_INTERFACE( ITriQuaternion );

BLUE_CLASS( TriVector ) :
	public ITriVector,
#if BLUE_WITH_PYTHON
	public IPythonMethods,
	public IPythonNumeric,
#endif
	public Vector3
{
public:
	EXPOSE_TO_BLUE();

	using ITriVector::Unlock;
	TriVector( IRoot* lockobj = NULL );
	~TriVector();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriVector
	/////////////////////////////////////////////////////////////////////////////////////
	void SetXYZ(
		float x,
		float y,
		float z );

	void SetVector(
		const Vector3* ar );

	const Vector3* GetVector() const;


	Vector3* CopyVector(
		Vector3 * in ) const;

	Vector3* Vector();

	float Length() const;

	float LengthSq() const;

	void Scale(
		float s );

	void Normalize();

	void TransformQuaternion(
		const Quaternion* in );

	void SetCrossProduct(
		const Vector3* v1,
		const Vector3* v2 );

	float DotProduct(
		const Vector3* v2 );

#if BLUE_WITH_PYTHON
	/////////////////////////////////////////////////////////////////////////////////////
	// IPythonMethods
	/////////////////////////////////////////////////////////////////////////////////////
	void Destroy();

	PyObject* GetAttr(
		const char* name,
		bool* handled );

	bool SetAttr(
		const char* name,
		PyObject* v,
		bool* handled );

	PyObject* Repr(
		bool* handled );

	/////////////////////////////////////////////////////////////////////////////////////
	// IPythonNumeric
	/////////////////////////////////////////////////////////////////////////////////////
	bool BinaryOp(
		PYNUMERIC_OPS op,
		IRoot * other,
		PyObject * *retval );

	bool UnaryOp(
		PYNUMERIC_OPS op,
		PyObject * *retval );

	void Coercion(
		PyObject * from,
		PyObject * *to );
#endif

public:
	void PyAdd( ITriVector * other );
	void PyCross( ITriVector * other );
	float PyDot( ITriVector * other );
	void PyLerp( ITriVector * other, float t );
	void PyMaximize( ITriVector * other );
	void PyMinimize( ITriVector * other );
	void PyProject(
		TriViewport * vp,
		ITriMatrix * project,
		ITriMatrix * view,
		ITriMatrix * world );
	void PySubtract( ITriVector * other );
	void PyTransformCoord( ITriMatrix * transform );
	void PyTransformNormal( ITriMatrix * transform );
	void PyUnproject(
		TriViewport * vp,
		ITriMatrix * project,
		ITriMatrix * view,
		ITriMatrix * world );
	void PyTransformQuaternion( ITriQuaternion * rotation );
	void PySetCrossProduct( ITriVector * v1, ITriVector * v2 );
	float PyDotProduct( ITriVector * other );
};

void GetNearestPointOnAABB( Vector3& out, const Vector3& p, const Vector3& min, const Vector3& max );

TYPEDEF_BLUECLASS( TriVector );


#endif