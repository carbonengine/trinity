// Copyright © 2000 CCP ehf.

#ifndef _EVECAMERA_H_
#define _EVECAMERA_H_

#define EVECAMERA_Description \
	"EveCamera defines a target camera, what is position and interest. There have \r\n\
been some requests for a free camera, position and orientation but that kind of \r\n\
camera behaviour will probably be handled by a separate object. EveCamera \r\n\
exposes m_pos m_intr m_viewVec which are updated every Render. This is done \r\n\
to make camera specific effect easy."


#include "TriObserverLocal.h"

BLUE_DECLARE( EveCamera );
BLUE_DECLARE( TriProjection );
BLUE_DECLARE( TriView );
BLUE_DECLARE_INTERFACE( ITriVectorFunction );
BLUE_DECLARE_INTERFACE( ITriScalarFunction );

class EveCamera : public INotify
{
public:
	static void CalculateProjectionMatrix( Matrix* mat, float aspectRatio, float fov, float offsetX, float offsetY, float front, float back, TriProjection* triProjection );
	static float CalculateFovFromProjection( const Matrix& mat );
	static Matrix ModifyClipPlanes( const Matrix& original, float nearClip, float farClip );
	static Matrix AddCenterOffset( const Matrix& original, float xOffset, float yOffset, float nearClip, float farClip );

	using INotify::Lock;
	using INotify::Unlock;

	EXPOSE_TO_BLUE();

	EveCamera( IRoot* lockobj = NULL );
	~EveCamera();

	/////////////////////////////////////////////////////////////////////////////////////
	void OrbitParent( float horizontal, float vertical );
	void SetOrbit( float yaw, float pitch );
	void RotateOnOrbit( float horizontal, float vertical );
	void SetRotationOnOrbit( float yaw, float pitch );

	void Dolly( float factor );
	void Zoom( Be::OptionalWithDefaultValue<int, -1> key );

	const TriViewPtr GetViewMatrix();
	const TriProjectionPtr GetProjection();

	const Vector3* GetPosition();

	void Update( Be::Time time );

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	/////////////////////////////////////////////////////////////////////////////////////
	bool OnModified( Be::Var* val );

	void ResetStartTime();

private:
	ITriVectorFunctionPtr m_parentTranslationCurve;
	ITriVectorFunctionPtr m_interestTranslationCurve;
	IBluePlacementObserverPtr m_audio2Listener;

	Vector3 m_translationFromParent;
	Quaternion m_rotationAroundParent;
	Quaternion m_rotationOfInterest;

	Vector3 m_alignment;
	Vector3 m_extraParentTranslation;
	bool m_useExtraParentTranslation;

	float m_fieldOfView;

	float m_frontClip;
	float m_backClip;

	float m_friction;
	float m_maxSpeed;

	float m_minPitch;
	float m_maxPitch;
	float m_minYaw;
	float m_maxYaw;

	float m_yawInt;
	float m_pitchInt;
	float m_yawIntSpeed;
	float m_pitchIntSpeed;

	float m_yaw;
	float m_pitch;
	float m_yawSpeed;
	float m_pitchSpeed;

	bool m_idleMove;
	float m_idleScale;
	float m_idleSpeed;

	bool m_noise;
	float m_noiseScale;
	float m_noiseDamp;

	float m_projectionCenterOffset;

	ITriScalarFunctionPtr m_zoomCurve;
	ITriScalarFunctionPtr m_noiseCurve;
	ITriScalarFunctionPtr m_noiseScaleCurve;
	ITriScalarFunctionPtr m_noiseDampCurve;

	bool m_update;
	int m_zoomKey;
	float m_zoomTime;

	float m_idleTheta;
	float m_NoiseyN;
	float m_NoisexN;
	float m_maxNoise;
	bool m_trackInterest;

	Vector3 m_pos;
	Vector3 m_intr;
	Vector3 m_viewVec;
	Vector3 m_rightVec;
	Vector3 m_upVec;

	//TODO the reason for "needing" this is the aspect ratio clamping
	// see comments in CalculateProjectionMatrix
	Matrix m_projectionTransform;
	TriProjectionPtr m_projectionMatrix;
	TriViewPtr m_viewMatrix;

	Be::Time m_time;
	Be::Time m_start;
	void CapPitchAndYaw();

	// Listener intended for providing error details if camera gets invalid values
	IBlueEventListenerPtr m_errorListener;
	bool m_failedLastFrame;
};
TYPEDEF_BLUECLASS( EveCamera );

#endif
