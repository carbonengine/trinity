// Copyright © 2019 CCP ehf.

#pragma once
#include "Eve/SpaceObject/EveSpaceObject2.h"


BLUE_DECLARE( EveSpaceObject2 );

namespace Tr2FollowCurveKeyInterpolation
{
enum Type
{
	// Constant (L0) interpolation
	CONSTANT = 0,
	// Linear (L1) interpolation
	LINEAR = 1,
	// Hermite/cubic (L2) interpolation
	HERMITE = 2,
};
}


BLUE_INTERFACE( ITr2FollowCurveKey ) :
	public IRoot
{
public:
	virtual Vector3 GetValue() = 0;
	virtual const float GetTime() = 0;
	virtual const Tr2FollowCurveKeyInterpolation::Type GetInterpolationType() = 0;
	virtual const Vector3 GetLeftTangent() = 0;
	virtual const Vector3 GetRightTangent() = 0;
};


BLUE_CLASS( Tr2ObjectFollowCurveKey ) :
	public ITr2FollowCurveKey,
	public INotify,
	public IInitialize
{
public:
	enum RotationSetting
	{
		NO_ROTATION,
		MODEL_ROTATION,
		LOCATOR_ROTATION
	};

	Tr2ObjectFollowCurveKey( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	~Tr2ObjectFollowCurveKey();

	// IInitialize
	bool Initialize();

	// INotify
	bool OnModified( Be::Var * value );

	// ITr2FollowCurveKey
	Vector3 GetValue();
	const float GetTime();
	const Tr2FollowCurveKeyInterpolation::Type GetInterpolationType();
	const Vector3 GetLeftTangent();
	const Vector3 GetRightTangent();

private:
	Locator* GetLocator();

	Matrix GetLocatorRotation();
	Matrix GetModelRotation();

	BlueSharedString m_name;

	float m_time;
	Vector3 m_leftTangent;
	Vector3 m_rightTangent;
	Vector3 m_rotatedLeftTangent;
	Vector3 m_rotatedRightTangent;
	Tr2FollowCurveKeyInterpolation::Type m_interpolation;

	IRootPtr m_object;
	BlueSharedString m_offsetLocatorName;
	Vector3 m_offset;
	RotationSetting m_rotationSetting;

	Locator* m_locator;
};
TYPEDEF_BLUECLASS( Tr2ObjectFollowCurveKey );


BLUE_CLASS( Tr2CameraFollowCurveKey ) :
	public ITr2FollowCurveKey,
	public INotify,
	public IInitialize
{
public:
	Tr2CameraFollowCurveKey( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	~Tr2CameraFollowCurveKey();

	// IInitialize
	bool Initialize();

	// INotify
	bool OnModified( Be::Var * value );

	// ITr2FollowCurveKey
	Vector3 GetValue();
	const float GetTime();
	const Tr2FollowCurveKeyInterpolation::Type GetInterpolationType();
	const Vector3 GetLeftTangent();
	const Vector3 GetRightTangent();

private:
	void CalculateBoxPosition();

	BlueSharedString m_name;

	float m_fovMultiplication;
	float m_angle;
	float m_angleZero;
	Vector3 m_objectBounds;
	Vector3 m_offset;

	float m_time;
	Vector3 m_leftTangent;
	Vector3 m_rightTangent;
	Vector3 m_rotatedLeftTangent;
	Vector3 m_rotatedRightTangent;
	Tr2FollowCurveKeyInterpolation::Type m_interpolation;

	Vector3 m_boxPosition;

	float m_frontClip;
	float m_fov;

	float m_minDistanceAlongViewAngle;
	float m_minDistanceFromViewAngle;

	bool m_enabled;
	float m_lastEnabledFOV;
	float m_lastEnabledFrontClip;
	Matrix m_lastEnabledInverseViewMatrix;
};
TYPEDEF_BLUECLASS( Tr2CameraFollowCurveKey );