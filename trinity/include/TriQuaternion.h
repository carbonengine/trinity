// Copyright © 2000 CCP ehf.

#ifndef _TRIQUATERNION_H_
#define _TRIQUATERNION_H_

#define TRIQUATERNION_Description \
	"Simple mapping of Quaternion over to Blue."

#include "ITriQuaternion.h"

#pragma warning( disable : 4275 )

BLUE_DECLARE( TriQuaternion );
BLUE_DECLARE_INTERFACE( ITriVector );
BLUE_DECLARE_INTERFACE( ITriMatrix );

#if BLUE_WITH_PYTHON
class TriQuaternion : public ITriQuaternion,
					  public IPythonMethods,
					  public Quaternion
{
public:
	EXPOSE_TO_BLUE();

	TriQuaternion( IRoot* lockobj = NULL );
	~TriQuaternion();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriQuaternion
	/////////////////////////////////////////////////////////////////////////////////////
	void SetXYZW(
		float x,
		float y,
		float z,
		float w );

	void SetQuaternion(
		const ::Quaternion* ar );

	const ::Quaternion* GetQuaternion() const;

	::Quaternion* CopyQuaternion(
		::Quaternion* in ) const;

	::Quaternion* Quaternion();

	void SetIdentity();

	void SetRotationAxis(
		const Vector3* axis,
		float angle );

	void GetRotationAxis(
		Vector3* axis,
		float* angle ) const;

	void SetYawPitchRoll(
		float yaw,
		float pitch,
		float roll );

	void GetYawPitchRoll(
		float* yaw,
		float* pitch,
		float* roll ) const;

	void IncreaseYawPitchRoll(
		float yaw,
		float pitch,
		float roll );

	void IncreaseLocalYawPitchRoll(
		float yaw,
		float pitch,
		float roll );

	void SetRotationArc(
		const Vector3* v0,
		const Vector3* v1 );

	void MultiplyQuaternion(
		const ::Quaternion* in );

	void SetSLERP(
		const ::Quaternion* q1,
		const ::Quaternion* q2,
		const float t );

	void Normalize();

	float Length() const;

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


public:
	void Py__init__( float _x, float _y, float _z, Be::Optional<float> _w );
	void PyIdentity();
	void PyInverse();
	float PyLength();
	void PyMultiply( ITriQuaternion* other );
	void PyNormalize();
	void PyRotationAxis( ITriVector* axis, float angle );

	void PySetRotationAxis( ITriVector* axis, float angle );
	Vector3 PyGetYawPitchRoll();
	void PyScale( float factor );
};
TYPEDEF_BLUECLASS( TriQuaternion );

#endif
#endif