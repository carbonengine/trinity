// Copyright © 2000 CCP ehf.

#ifndef _ITRIQUATERNION_H_
#define _ITRIQUATERNION_H_

struct Quaternion;
struct Vector3;

BLUE_INTERFACE( ITriQuaternion ) :
	public IRoot
{
	// Construction and data access
	virtual void SetXYZW(
		float x,
		float y,
		float z,
		float w ) = 0;

	virtual void SetQuaternion(
		const Quaternion* ar ) = 0;

	virtual const ::Quaternion* GetQuaternion() const = 0;

	virtual ::Quaternion* CopyQuaternion(
		Quaternion * in ) const = 0;

	virtual ::Quaternion* Quaternion() = 0;


	// Math functions
	virtual void SetIdentity() = 0;

	virtual void SetRotationAxis(
		const Vector3* axis,
		float angle ) = 0;

	virtual void GetRotationAxis(
		Vector3 * axis,
		float* angle ) const = 0;

	virtual void SetYawPitchRoll(
		float yaw,
		float pitch,
		float roll ) = 0;

	virtual void GetYawPitchRoll(
		float* yaw,
		float* pitch,
		float* roll ) const = 0;

	virtual void IncreaseYawPitchRoll(
		float yaw,
		float pitch,
		float roll ) = 0;

	virtual void IncreaseLocalYawPitchRoll(
		float yaw,
		float pitch,
		float roll ) = 0;

	virtual void SetRotationArc(
		const Vector3* v0,
		const Vector3* v1 ) = 0;

	virtual void MultiplyQuaternion(
		const ::Quaternion* in ) = 0;

	virtual void SetSLERP(
		const ::Quaternion* q1,
		const ::Quaternion* q2,
		const float t ) = 0;

	virtual void Normalize() = 0;

	virtual float Length() const = 0;
};

#endif