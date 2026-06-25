// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Eve/EveCamera.h"
#include "TriDevice.h"
#include "TriProjection.h"
#include "TriView.h"
#include "Include/TriMath.h"
#include "Curves/Tr2CurveScalar.h"

static const float defFOV = TRI_PIBY2;

static inline float CutoffYawPitch( float value, float speed )
{
	// This helps avoid jitter caused by spring equations.
	const float cutoff = 0.0001f;
	if( fabs( value - speed ) < cutoff )
	{
		return speed;
	}
	return value;
}

EveCamera::EveCamera( IRoot* lockobj ) :
	m_friction( 7.0f ),
	m_maxSpeed( 0.05f ),
	m_zoomKey( 0 ),
	m_zoomTime( 0.0f ),

	m_fieldOfView( defFOV ),
	m_frontClip( 10.0f ),
	m_backClip( 10000000.0f ),
	// if TRI_PIBY2 is used everything gets borken at the top :-( ?!?!
	m_minPitch( -1.4f ),
	m_maxPitch( 1.4f ),
	m_minYaw( 0.0f ),
	m_maxYaw( 0.0f ),

	m_noise( false ),
	m_noiseScale( 1.0f ),
	m_noiseDamp( 1.1f ),

	m_idleMove( false ),
	m_idleScale( 2.0f ),
	m_idleSpeed( 0.8f ),

	m_yawInt( 0.0f ),
	m_pitchInt( 0.0f ),
	m_yawIntSpeed( 0.0f ),
	m_pitchIntSpeed( 0.0f ),

	m_idleTheta( 0.0f ),
	m_trackInterest( false ),
	m_NoiseyN( 0.0f ),
	m_NoisexN( 0.0f ),
	m_maxNoise( 80.0f ),

	m_yaw( 0.0f ),
	m_pitch( 0.0f ),
	m_yawSpeed( 0.0f ),
	m_pitchSpeed( 0.0f ),
	m_time( 0 ),
	m_start( 0 ),

	m_projectionCenterOffset( 0.0f ),
	m_rotationAroundParent( 0.f, 0.f, 0.f, 1.f ),
	m_rotationOfInterest( 0.f, 0.f, 0.f, 1.f ),
	m_translationFromParent( 0, 0, 20.0f ),

	m_pos( 0, 0, 0 ),
	m_intr( 0, 0, 0 ),
	m_viewVec( 0, 0, 0 ),
	m_rightVec( 0, 0, 0 ),
	m_upVec( 0, 0, 0 ),
	m_alignment( 0, 1, 0 ),

	m_extraParentTranslation( 0, 0, 0 ),
	m_useExtraParentTranslation( false ),

	m_update( true ),
	m_failedLastFrame( false ),
	m_projectionTransform( IdentityMatrix() )
{
	Tr2CurveScalarPtr zoomCurve;
	zoomCurve.CreateInstance();

	zoomCurve->AddKey( 0.0f, defFOV, Tr2CurveInterpolation::HERMITE, 0.0f, -11.0f, Tr2CurveTangentType::FREE_SPLIT );
	zoomCurve->AddKey( 0.225f, 0.8f, Tr2CurveInterpolation::HERMITE, 0.0f, -9.0f, Tr2CurveTangentType::FREE_SPLIT );
	zoomCurve->AddKey( 0.45f, 0.1f, Tr2CurveInterpolation::HERMITE, 0.0f, 20.0f, Tr2CurveTangentType::FREE_SPLIT );
	zoomCurve->AddKey( 0.675f, defFOV, Tr2CurveInterpolation::HERMITE, 0.0f, 0.0f, Tr2CurveTangentType::FREE_SPLIT );

	m_zoomCurve = zoomCurve;

	m_projectionMatrix.CreateInstance();
	m_viewMatrix.CreateInstance();
}


EveCamera::~EveCamera()
{
}


void EveCamera::OrbitParent( float horizontal, float vertical )
{
	//Accumulates Yaw and Pitch angles
	float oldYaw = m_yawSpeed;
	float oldPitch = m_pitchSpeed;

	m_yawSpeed += m_maxSpeed * horizontal;
	m_pitchSpeed -= m_maxSpeed * vertical;

	// Pitch is clamped, but the speed variable is allowed to go way beyond limits
	// when going above or below thresholds, but if on the way back, assume it
	// was exactly at the threshold. This is to have consistent spring responsiveness
	if( m_pitchSpeed > m_maxPitch && m_pitchSpeed - oldPitch < 0.0f )
		m_pitchSpeed = m_maxPitch;
	else if( m_pitchSpeed < m_minPitch && oldPitch - m_pitchSpeed < 0.0f )
		m_pitchSpeed = m_minPitch;

	if( m_minYaw != m_maxYaw )
	{
		// Yaw limits specified, so clamp yaw as well
		if( m_yawSpeed > m_maxYaw && m_yawSpeed - oldYaw < 0.0f )
			m_yawSpeed = m_maxYaw;
		else if( m_yawSpeed < m_minYaw && oldYaw - m_yawSpeed < 0.0f )
			m_yawSpeed = m_minYaw;
	}
}

void EveCamera::SetOrbit( float yaw, float pitch )
{
	m_yaw = yaw;
	m_pitch = pitch;

	m_yawSpeed = m_yaw;
	m_pitchSpeed = m_pitch;

	m_yawSpeed = fmodf( m_yawSpeed, TRI_2PI );
	m_yaw = fmodf( m_yaw, TRI_2PI );
}

void EveCamera::SetRotationOnOrbit( float yaw, float pitch )
{
	m_yawInt = yaw;
	m_pitchInt = pitch;

	m_yawIntSpeed = yaw;
	m_pitchIntSpeed = pitch;

	m_rotationOfInterest = RotationQuaternion( m_yawInt, m_pitchInt, 0.0f );
}

void EveCamera::RotateOnOrbit( float horizontal, float vertical )
{
	//Accumulates Yaw and Pitch angles
	float oldYaw = m_yawIntSpeed;
	float oldPitch = m_pitchIntSpeed;

	m_yawIntSpeed += m_maxSpeed * horizontal;
	m_pitchIntSpeed -= m_maxSpeed * vertical;

	// Pitch is clamped, but the speed variable is allowed to go way beyond limits
	// when going above or below thresholds, but if on the way back, assume it
	// was exactly at the threshold. This is to have consistent spring responsiveness
	if( m_pitchIntSpeed > m_maxPitch && m_pitchIntSpeed - oldPitch < 0.0f )
		m_pitchIntSpeed = m_maxPitch;
	else if( m_pitchIntSpeed < m_minPitch && oldPitch - m_pitchIntSpeed < 0.0f )
		m_pitchIntSpeed = m_minPitch;

	if( m_minYaw != m_maxYaw )
	{
		// Yaw limits specified, so clamp yaw as well
		if( m_yawIntSpeed > m_maxYaw && m_yawIntSpeed - oldYaw < 0.0f )
			m_yawIntSpeed = m_maxYaw;
		else if( m_yawIntSpeed < m_minYaw && oldYaw - m_yawIntSpeed < 0.0f )
			m_yawIntSpeed = m_minYaw;
	}

	m_rotationOfInterest = RotationQuaternion( m_yawInt, m_pitchInt, 0.0f );
}


void EveCamera::Dolly( float factor )
{
	m_translationFromParent.z += factor;
}


void EveCamera::Zoom( Be::OptionalWithDefaultValue<int, -1> key )
{
	ssize_t size = 0;
	if( Tr2CurveScalarPtr curve = BlueCastPtr( m_zoomCurve ) )
	{
		size = ssize_t( curve->GetKeys().size() );
	}
	if( size < 1 )
	{
		CCP_LOGERR( "Tried to zoom but the camera had no keys on zoomCurve" );
		return;
	}
	if( key != -1 )
		m_zoomKey = key;
	else
		m_zoomKey++;

	if( m_zoomKey >= ( size - 1 ) )
		m_zoomKey = 0;

	if( Tr2CurveScalarPtr curve = BlueCastPtr( m_zoomCurve ) )
	{
		m_zoomTime = curve->GetKeys()[m_zoomKey].m_time;
	}
}

const TriViewPtr EveCamera::GetViewMatrix()
{
	return m_viewMatrix;
}

const TriProjectionPtr EveCamera::GetProjection()
{
	return m_projectionMatrix;
}

const Vector3* EveCamera::GetPosition()
{
	return &m_pos;
}

void EveCamera::Update( Be::Time t )
{
	if( !m_update )
		return;
	bool failed = false;

	if( m_start == 0 )
	{
		m_start = t;
	}

	// 'dT' is the time span since this function was last called
	float dT = float( TimeAsDouble( BeOS->GetInfo()->mRealTime - m_time ) );
	m_time = BeOS->GetInfo()->mRealTime;

	Vector3 parentPosition( 0, 0, 0 );
	if( m_parentTranslationCurve )
	{
		m_parentTranslationCurve->GetValueAt( &parentPosition, t );
	}

	if( m_useExtraParentTranslation )
	{
		parentPosition += m_extraParentTranslation;
	}

	// Limit camera translation by parent bounding radius
	if( m_translationFromParent.z < max( 1.f, m_frontClip ) )
	{
		m_translationFromParent.z = max( 1.f, m_frontClip );
	}
	if( !IsFinite( m_translationFromParent ) )
	{
		if( !m_failedLastFrame )
		{
			CCP_LOGERR( "EveCamera: m_translationFromParent not finite(%f, %f, %f). Setting to safe value.",
						m_translationFromParent.x,
						m_translationFromParent.y,
						m_translationFromParent.z );
		}
		m_translationFromParent = Vector3( 0, 0, 1 );
		failed = true;
	}

	if( Tr2CurveScalarPtr curve = BlueCastPtr( m_zoomCurve ) )
	{
		if( curve->Length() > 0.0f )
		{
			if( ( ssize_t( curve->GetKeys().size() ) > ( m_zoomKey + 1 ) ) && ( m_zoomTime < curve->GetKeys()[m_zoomKey + 1].m_time ) )
			{
				m_zoomTime += (float)dT;
				if( m_zoomTime > curve->GetKeys()[m_zoomKey + 1].m_time )
					m_zoomTime = curve->GetKeys()[m_zoomKey + 1].m_time;
				m_fieldOfView = curve->Update( m_zoomTime );
			}
		}
	}

	// Cap the pitch input at twich the max(to keep some pressure)
	m_yaw = ( m_yaw + m_friction * dT * m_yawSpeed ) / ( 1.0f + m_friction * dT );
	m_yaw = CutoffYawPitch( m_yaw, m_yawSpeed );
	m_pitch = ( m_pitch + m_friction * dT * m_pitchSpeed ) / ( 1.0f + m_friction * dT );
	m_pitch = CutoffYawPitch( m_pitch, m_pitchSpeed );

	CapPitchAndYaw();

	// Find absolute camera position
	m_rotationAroundParent = RotationQuaternion( m_yaw, m_pitch, 0.0f );
	Vector3 vecCamPos;
	TriVectorRotateQuaternion( &vecCamPos, &m_translationFromParent, &m_rotationAroundParent );

	vecCamPos += parentPosition;


	////////////////////////////////////////////////////////////////////////
	// Calculate perputal idle motion
	////////////////////////////////////////////////////////////////////////

	// Use the 'Eight' curve
	m_idleTheta += dT * m_idleSpeed;
	if( m_idleTheta > TRI_2PI )
	{
		m_idleTheta = fmodf( m_idleTheta, TRI_2PI );
	}

	float idleYaw = 0.0f, idlePitch = 0.0f;
	if( m_idleMove )
	{
		idleYaw = m_idleScale * cosf( m_idleTheta );
		idlePitch = 1.2f * idleYaw * sinf( m_idleTheta );
	}

	if( m_noiseCurve )
	{
		m_noise = m_noiseCurve->Update( TimeAsDouble( t - m_start ) ) > 0.0f;
	}
	else
	{
		m_noise = false;
	}
	if( m_noise )
	{
		if( m_noiseScaleCurve )
		{
			float noiseScale = m_noiseScaleCurve->Update( TimeAsDouble( t - m_start ) );
			if( IsFinite( noiseScale ) )
			{
				m_noiseScale = noiseScale;
			}
			else
			{
				CCP_LOGERR( "EveCamera: Got non-finite noise scale." );
			}
		}
		if( m_noiseDampCurve )
		{
			float noiseDamp = m_noiseDampCurve->Update( TimeAsDouble( t - m_start ) );
			if( IsFinite( noiseDamp ) )
			{
				m_noiseDamp = noiseDamp;
			}
			else
			{
				CCP_LOGERR( "EveCamera: Got non-finite noise damp." );
			}
		}
		float ran = TriRand() - 0.5f;
		m_NoisexN = ( m_NoisexN + m_noiseDamp * ran ) / ( 1.0f + m_noiseDamp * dT );
		m_NoisexN = TriClamp( m_NoisexN, -m_maxNoise, m_maxNoise );
		ran = TriRand() - 0.5f;
		m_NoiseyN = ( m_NoiseyN + m_noiseDamp * ran ) / ( 1.0f + m_noiseDamp * dT );
		m_NoiseyN = TriClamp( m_NoiseyN, -m_maxNoise, m_maxNoise );

		idleYaw += m_noiseScale * m_NoisexN;
		idlePitch += m_noiseScale * m_NoiseyN;
	}

	// Find the interest point of the camera (probably the ship)
	Vector3 vecCamIntr( parentPosition );
	Vector3 toInterest = vecCamIntr - vecCamPos;
	Vector3 side;
	Vector3 up = Vector3( 0.0f, 1.0f, 0.0f );

	toInterest = Normalize( toInterest );
	//draw a line through the interest, 100 meters long
	Vector3 extendedInterest = toInterest * 100.0f;

	side = Cross( toInterest, up );
	up = Cross( side, toInterest );

	//all noise should be added to the interest point at 200 meters
	//so that the size of the interest won't affect the perceived noise (though it should)
	extendedInterest += idleYaw * side + idlePitch * up;

	// Add camera noise
	vecCamIntr = vecCamPos + extendedInterest;



	///////////////////////////////////////////////////////////////////////
	// And Finally animate the rotation around the 'other' interest if present
	////////////////////////////////////////////////////////////////////////
	m_yawInt = ( m_yawInt + m_friction * dT * m_yawIntSpeed ) / ( 1.0f + m_friction * dT );
	m_yawInt = CutoffYawPitch( m_yawInt, m_yawIntSpeed );
	m_pitchInt = ( m_pitchInt + m_friction * dT * m_pitchIntSpeed ) / ( 1.0f + m_friction * dT );
	m_pitchInt = CutoffYawPitch( m_pitchInt, m_pitchIntSpeed );

	// check if we are tracking an interest, in which
	// case determine its yaw and pitch and make that the target
	if( m_trackInterest && m_interestTranslationCurve )
	{
		Vector3 aPos;
		m_interestTranslationCurve->GetValueAt( &aPos, t );
		float aYaw, aPitch, r;
		aPos = aPos - vecCamPos;
		Vector3 tPos;

		tPos.x = Dot( aPos, side );
		tPos.y = Dot( aPos, up );
		tPos.z = Dot( aPos, toInterest );
		r = Length( tPos );
		if( r > 0.0f )
			aPitch = asinf( tPos.y / r );
		else
			aPitch = 0.0f;

		aYaw = atan2f( tPos.x, tPos.z );

		m_yawIntSpeed = -aYaw;
		m_pitchIntSpeed = aPitch;
	}
	else if( m_rotationOfInterest.x == 0.0f && m_rotationOfInterest.y == 0.0f && m_rotationOfInterest.z == 0.0f && m_rotationOfInterest.w == 1.0f )
	{
		m_yawIntSpeed = m_pitchIntSpeed = 0.0f;
	}


	// Cap both yat and pitch
	if( m_pitchInt > m_maxPitch )
		m_pitchInt = m_maxPitch;
	else if( m_pitchInt < m_minPitch )
		m_pitchInt = m_minPitch;

	if( m_minYaw != m_maxYaw )
	{
		if( m_yawInt > m_maxYaw )
			m_yawInt = m_maxYaw;
		else if( m_yaw < m_minYaw )
			m_yawInt = m_minYaw;
	}

	m_rotationOfInterest = RotationQuaternion( m_yawInt, m_pitchInt, 0.0f );

	////////////////////////////////////////////////////////////////////////
	// Set up the camera at the calculated position and with the calculated interest
	////////////////////////////////////////////////////////////////////////

	// We transform the m_alignment direction to avoid having to worry about the view direction
	// and the alignemnt being parallel
	Vector3 realUpDir;
	TriVectorRotateQuaternion( &realUpDir, &m_alignment, &m_rotationAroundParent );
	realUpDir = Normalize( realUpDir );

	if( !IsFinite( realUpDir ) || !LengthSq( realUpDir ) )
	{
		if( !m_failedLastFrame )
		{
			CCP_LOGERR( "EveCamera: Invalid up direction (%f, %f, %f), rotation (%f, %f, %f, %f)",
						realUpDir.x,
						realUpDir.y,
						realUpDir.z,
						m_rotationAroundParent.x,
						m_rotationAroundParent.y,
						m_rotationAroundParent.z,
						m_rotationAroundParent.w );
		}
		realUpDir = Vector3( 0, 1, 0 );
		failed = true;
	}

	Vector3 lookAtLength = vecCamPos - vecCamIntr;
	if( !Length( lookAtLength ) )
	{
		if( !m_failedLastFrame )
		{
			CCP_LOGERR( "EveCamera: Camera position and camera interest identical(%f, %f, %f).",
						vecCamPos.x,
						vecCamPos.y,
						vecCamPos.z );
		}
		vecCamPos += m_translationFromParent;
		failed = true;
	}
	if( !IsFinite( vecCamIntr ) || !IsFinite( vecCamPos ) )
	{
		if( !m_failedLastFrame )
		{
			CCP_LOGERR( "EveCamera: Camera interest(%f, %f, %f) and/or position(%f, %f, %f) invalid.",
						vecCamIntr.x,
						vecCamIntr.y,
						vecCamIntr.z,
						vecCamPos.x,
						vecCamPos.y,
						vecCamPos.z );
		}
		vecCamIntr = Vector3( 0, 0, 0 );
		vecCamPos = m_translationFromParent;
		failed = true;
	}

	Matrix viewTransform, viewTransformInt;
	viewTransform = LookAtMatrix( vecCamPos, vecCamIntr, realUpDir );
	// now rotate toward other interest
	Quaternion tmpResult = Inverse( m_rotationOfInterest );
	TriMatrixRotate( &viewTransformInt, &viewTransform, &tmpResult );

	CalculateProjectionMatrix( &m_projectionTransform, gTriDev->AspectRatio(), m_fieldOfView, m_projectionCenterOffset, 0, m_frontClip, m_backClip, m_projectionMatrix );


	// update cached attribs
	m_pos = vecCamPos;
	m_intr = vecCamIntr;
	m_viewVec = Vector3( viewTransformInt._13, viewTransformInt._23, viewTransformInt._33 );
	m_upVec = Vector3( viewTransformInt._12, viewTransformInt._22, viewTransformInt._32 );
	m_rightVec = Vector3( viewTransformInt._11, viewTransformInt._21, viewTransformInt._31 );

	if( Length( m_viewVec ) && Length( m_upVec ) && Length( m_rightVec ) )
	{
		m_viewMatrix->SetTransform( viewTransformInt );
	}
	else
	{
		if( !m_failedLastFrame )
		{
			CCP_LOGERR( "EveCamera: Invalid results after applying rotation of interest. Ignoring interest." );
		}
		failed = true;

		m_viewVec = Vector3( viewTransform._13, viewTransform._23, viewTransform._33 );
		m_upVec = Vector3( viewTransform._12, viewTransform._22, viewTransform._32 );
		m_rightVec = Vector3( viewTransform._11, viewTransform._21, viewTransform._31 );

		m_viewMatrix->SetTransform( viewTransform );
	}

	if( m_audio2Listener )
	{
		m_audio2Listener->UpdatePlacement( m_viewVec, m_upVec, m_pos );
	}
	if( m_errorListener && failed && !m_failedLastFrame )
	{
		m_errorListener->HandleEvent( nullptr );
	}
	m_failedLastFrame = failed;
}


bool EveCamera::OnModified(
	Be::Var* value )
{
	if( IsMatch( value, m_rotationAroundParent ) )
	{
		float unused;
		TriQuaternionToYawPitchRoll( &m_yaw, &m_pitch, &unused, &m_rotationAroundParent );
	}
	else if( IsMatch( value, m_interestTranslationCurve ) )
	{
		if( m_interestTranslationCurve && ( m_interestTranslationCurve != m_parentTranslationCurve ) )
		{
			m_trackInterest = true;
		}
		else
		{
			m_trackInterest = false;
			m_yawIntSpeed = 0.0f;
			m_pitchIntSpeed = 0.0f;
		}
	}

	return true;
}

void EveCamera::CapPitchAndYaw()
{
	if( m_pitch > m_maxPitch )
		m_pitch = m_maxPitch;
	else if( m_pitch < m_minPitch )
		m_pitch = m_minPitch;

	if( m_minYaw != m_maxYaw )
	{
		if( m_yaw > m_maxYaw )
			m_yaw = m_maxYaw;
		else if( m_yaw < m_minYaw )
			m_yaw = m_minYaw;
	}
}

// -------------------------------------------------------------
// Description:
//   Sets up and assigns the desired projection to the matrix
//   provided and passes it on to the projection if it's provided.
// Arguments:
//   mat - the transform matrix that stores the result
//   aspectRatio - the aspect ratio for this projection matrix
//   fov - the desired field of view
//   centerOffset - camera projection offset(horizontal offset)
//   front - the front clipping plane
//   back - the back clipping plane
//   projection - the TriProjection matrix to store the resulting matrix
// -------------------------------------------------------------
void EveCamera::CalculateProjectionMatrix( Matrix* mat, float aspectRatio, float fov, float offsetX, float offsetY, float front, float back, TriProjection* projection )
{
	if( !aspectRatio || !IsFinite( aspectRatio ) )
	{
		aspectRatio = 1;
	}
	if( !fov || !IsFinite( fov ) )
	{
		fov = 1;
	}

	// TODO: update TriProjection to do all this for us, having the aspect ratio correction as a value on the
	// TriProjection itself rather than transmogrifying the matrix here.
	float dX = aspectRatio * front * tan( fov / 2 );
	float dY = front * tan( fov / 2 );

	if( aspectRatio > 1.6f )
	{
		// The field of view in x is now too warped so we switch things around and
		// adjust the matrix so that the fov in x doesn't go beyond what it would be
		// at a 1.6 aspect ratio.  Note that 1920/1200 = 1.6.
		float adjustment = aspectRatio / 1.6f;
		dX /= adjustment;
		dY /= adjustment;
	}
	float left = -dX + dX * offsetX;
	float right = dX + dX * offsetX;
	float top = dY + dY * offsetY;
	float bottom = -dY + dY * offsetY;
	*mat = PerspectiveOffCenterMatrix( left, right, bottom, top, front, back );

	if( projection )
	{
		// TODO: as per comment at the top here. Should call something like
		// projection->OffCenterAspectAdjust(left, right, -dy, dy, front, back, aspectClamp(1.6 in this case))
		projection->CustomProjection( (Matrix)*mat );
	}
}


// -------------------------------------------------------------
// Description:
//   Extracts the field of view from the projection matrix. It's
//   assumed to have been generated via the CalculateProjectionMatrix
//   method.
// Arguments:
//   mat - the transform matrix that stores the result
// Return Value:
//   the field of view used to generate the mat projection matrix
// -------------------------------------------------------------
float EveCamera::CalculateFovFromProjection( const Matrix& mat )
{
	float aspectRatio = ( mat._11 ? mat._22 / mat._11 : 0.0f );
	float aspectAdjustment = 1.0f;
	if( aspectRatio > 1.6f )
	{
		aspectAdjustment = aspectRatio / 1.6f;
	}

	return 2.0f * atan( aspectAdjustment / mat._22 );
}

Matrix EveCamera::ModifyClipPlanes( const Matrix& original, float nearClip, float farClip )
{
	Matrix projection( original );
	float aspectRatio = ( projection._11 ? projection._22 / projection._11 : 0.0f );

	float fov = EveCamera::CalculateFovFromProjection( projection );

	// Now we have to see if our projection is off center.
	float centerOffsetX = original._31;
	float centerOffsetY = original._32;

	EveCamera::CalculateProjectionMatrix( &projection, aspectRatio, fov, centerOffsetX, centerOffsetY, nearClip, farClip, NULL );

	return projection;
}

Matrix EveCamera::AddCenterOffset( const Matrix& original, float xOffset, float yOffset, float nearClip, float farClip )
{
	Matrix projection( original );
	float aspectRatio = ( projection._11 ? projection._22 / projection._11 : 0.0f );

	float fov = EveCamera::CalculateFovFromProjection( projection );

	// Now we have to see if our projection is off center.
	float centerOffsetX = original._31 + xOffset;
	float centerOffsetY = original._32 + yOffset;

	EveCamera::CalculateProjectionMatrix( &projection, aspectRatio, fov, centerOffsetX, centerOffsetY, nearClip, farClip, NULL );

	return projection;
}

void EveCamera::ResetStartTime()
{
	m_start = 0;
}