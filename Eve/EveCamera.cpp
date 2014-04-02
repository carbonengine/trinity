#include "StdAfx.h"
#include "Eve/EveCamera.h"
#include "TriDevice.h"
#include "TriProjection.h"
#include "TriView.h"
#include "TriError.h"
#include "Include/TriMath.h"

const float defFOV = TRI_PIBY2;
const float cutoff = 0.00001f;

EveCamera::EveCamera(IRoot* lockobj) :
	m_friction    ( 7.0f  ),
	m_maxSpeed    ( 0.05f ),
	m_zoomKey     ( 0     ),
	m_zoomTime    ( 0.0f  ),

	m_fieldOfView ( defFOV   ),
	m_frontClip   ( 10.0f     ),
	m_backClip    ( 10000000.0f ),
	// if TRI_PIBY2 is used everything gets borken at the top :-( ?!?!
	m_minPitch (  -1.4f ),
	m_maxPitch (   1.4f ),
	m_minYaw   (  0.0f  ),
	m_maxYaw   (   0.0f ),

	m_noise		( false),
	m_noiseScale	(1.0f),
	m_noiseDamp	(1.1f),

	m_idleMove			( false ),
	m_idleScale		( 2.0f  ),
	m_idleSpeed		( 0.8f  ),

	m_yawInt			(0.0f),
	m_pitchInt		(0.0f),
	m_yawIntSpeed	(0.0f),
	m_pitchIntSpeed	(0.0f),

	m_idleTheta		( 0.0f ),
	m_trackInterest	( false ),
	m_NoiseyN		( 0.0f ),
	m_NoisexN		( 0.0f ),
	m_maxNoise		( 80.0f ),

	m_yaw         ( 0.0f ),
	m_pitch       ( 0.0f ),
	m_yawSpeed    ( 0.0f ),
	m_pitchSpeed	 ( 0.0f ),
	m_time        ( 0    ),

	m_projectionCenterOffset(0.0f),
	m_rotationAroundParent( 0.f, 0.f, 0.f, 1.f ),
	m_rotationOfInterest( 0.f, 0.f, 0.f, 1.f ),
	m_translationFromParent(0, 0, 20.0f),

	m_pos(0, 0, 0),
	m_intr(0, 0, 0),
	m_viewVec(0, 0, 0),
	m_rightVec(0, 0, 0),
	m_upVec(0, 0, 0),
	m_alignment(0, 1, 0),

	m_extraParentTranslation(0, 0, 0),
	m_useExtraParentTranslation( false ),

	PARENTLOCK( m_zoomCurve ),
	m_update(true)
{

	m_zoomCurve.AddKey( 0.0f,   defFOV, 0.0f,-11.0f, TRIINT_HERMITE );
	m_zoomCurve.AddKey( 0.225f,   0.8f, 0.0f, -9.0f, TRIINT_HERMITE );
	m_zoomCurve.AddKey( 0.45f,    0.1f, 0.0f, 20.0f, TRIINT_HERMITE );
	m_zoomCurve.AddKey( 0.675f, defFOV, 0.0f,  0.0f, TRIINT_HERMITE );
	m_zoomCurve.Sort();

	D3DXMatrixIdentity(&m_projectionTransform);
	m_projectionMatrix.CreateInstance();
	m_viewMatrix.CreateInstance();
}


EveCamera::~EveCamera()
{
}


void EveCamera::OrbitParent(float horizontal, float vertical)
{
	//Accumulates Yaw and Pitch angles
	float oldYaw = m_yawSpeed;
	float oldPitch = m_pitchSpeed;

	m_yawSpeed += m_maxSpeed*horizontal;
	m_pitchSpeed -= m_maxSpeed*vertical;

	// Pitch is clamped, but the speed variable is allowed to go way beyond limits
	// when going above or below thresholds, but if on the way back, assume it
	// was exactly at the threshold. This is to have consistent spring responsiveness
	if(m_pitchSpeed > m_maxPitch && m_pitchSpeed - oldPitch < 0.0f)
		m_pitchSpeed = m_maxPitch;
	else if(m_pitchSpeed < m_minPitch && oldPitch-m_pitchSpeed < 0.0f)
		m_pitchSpeed = m_minPitch;

	if(m_minYaw != m_maxYaw){
		// Yaw limits specified, so clamp yaw as well
		if(m_yawSpeed > m_maxYaw && m_yawSpeed - oldYaw < 0.0f)
			m_yawSpeed = m_maxYaw;
		else if(m_yawSpeed < m_minYaw && oldYaw-m_yawSpeed < 0.0f)
			m_yawSpeed = m_minYaw;
	}

}

void EveCamera::SetOrbit(float yaw, float pitch)
{
	m_yaw = yaw;
	m_pitch = pitch;

	m_yawSpeed = m_yaw;
	m_pitchSpeed = m_pitch;

	m_yawSpeed = fmodf(m_yawSpeed,TRI_2PI);
	m_yaw = fmodf(m_yaw,TRI_2PI);
}

void EveCamera::SetRotationOnOrbit(float yaw, float pitch)
{
	m_yawInt = yaw;
	m_pitchInt = pitch;

	m_yawIntSpeed = yaw;
	m_pitchIntSpeed = pitch;

	D3DXQuaternionRotationYawPitchRoll(&m_rotationOfInterest, m_yawInt, m_pitchInt, 0.0f);
}

void EveCamera::RotateOnOrbit(float horizontal, float vertical)
{
	//Accumulates Yaw and Pitch angles
	float oldYaw = m_yawIntSpeed;
	float oldPitch = m_pitchIntSpeed;

	m_yawIntSpeed += m_maxSpeed*horizontal;
	m_pitchIntSpeed -= m_maxSpeed*vertical;

	// Pitch is clamped, but the speed variable is allowed to go way beyond limits
	// when going above or below thresholds, but if on the way back, assume it
	// was exactly at the threshold. This is to have consistent spring responsiveness
	if(m_pitchIntSpeed > m_maxPitch && m_pitchIntSpeed - oldPitch < 0.0f)
		m_pitchIntSpeed = m_maxPitch;
	else if(m_pitchIntSpeed < m_minPitch && oldPitch-m_pitchIntSpeed < 0.0f)
		m_pitchIntSpeed = m_minPitch;

	if(m_minYaw != m_maxYaw){
		// Yaw limits specified, so clamp yaw as well
		if(m_yawIntSpeed > m_maxYaw && m_yawIntSpeed - oldYaw < 0.0f)
			m_yawIntSpeed = m_maxYaw;
		else if(m_yawIntSpeed < m_minYaw && oldYaw-m_yawIntSpeed < 0.0f)
			m_yawIntSpeed = m_minYaw;
	}
	
	D3DXQuaternionRotationYawPitchRoll(&m_rotationOfInterest, m_yawInt, m_pitchInt, 0.0f);
}


void EveCamera::Dolly(float factor)
{
	m_translationFromParent.z += factor;
}


void EveCamera::Zoom( Be::OptionalWithDefaultValue<int, -1> key )
{
	if (m_zoomCurve.mKeys.GetSize() < 1)
	{
		TriError::ReportError( BEDEF, Clsid(), 
			"\r\nTried to zoom but the camera had no keys on zoomCurve"
			);
		return;
	}
    if (key != -1)
		m_zoomKey = key;
	else
		m_zoomKey++;

	if(m_zoomKey >= (m_zoomCurve.mKeys.GetSize() - 1) )
		m_zoomKey = 0;	

	m_zoomTime = m_zoomCurve.mKeys[m_zoomKey]->mTime;
}

const TriViewPtr EveCamera::GetViewMatrix()
{
	return m_viewMatrix;
}

const Vector3* EveCamera::GetPosition(
	)
{
	return &m_pos;
}

void EveCamera::Update( Be::Time t )
{	
	if(!m_update)
		return;

	// 'dT' is the time span since this function was last called
	float dT = float(TimeAsDouble(BeOS->GetInfo()->mRealTime - m_time));
	m_time = BeOS->GetInfo()->mRealTime;

	Vector3 parentPosition(0, 0, 0);
	if (m_parentTranslationCurve)
	{
		m_parentTranslationCurve->GetValueAt(&parentPosition, t);
	}

	if (m_useExtraParentTranslation)
	{
		parentPosition += m_extraParentTranslation;
	}

	// Limit camera translation by parent bounding radius
	if ( m_translationFromParent.z < max(1.f, m_frontClip) )
	{
		m_translationFromParent.z =  max(1.f, m_frontClip);
	}
	if ( !IsFinite( m_translationFromParent ) )
	{
		CCP_LOGERR( "EveCamera: m_translationFromParent not finite(%f, %f, %f). Setting to safe value.", 
			m_translationFromParent.x, m_translationFromParent.y, m_translationFromParent.z );
		m_translationFromParent = Vector3(0, 0, 1);
	}

	if(m_zoomCurve.mLength > 0.0f)
	{
		if((m_zoomCurve.mKeys.GetSize() > (m_zoomKey+1)) && (m_zoomTime < m_zoomCurve.mKeys[m_zoomKey+1]->mTime))
		{
			m_zoomTime += (float)dT;
			if(m_zoomTime > m_zoomCurve.mKeys[m_zoomKey+1]->mTime)
				m_zoomTime = m_zoomCurve.mKeys[m_zoomKey+1]->mTime;
			m_fieldOfView = m_zoomCurve.Update(m_zoomTime);
		}
	}

	// Cap the pitch input at twich the max(to keep some pressure)
	m_yaw = (m_yaw + m_friction * dT * m_yawSpeed) / (1.0f + m_friction * dT);
	m_pitch = (m_pitch + m_friction * dT * m_pitchSpeed)/(1.0f + m_friction * dT);
	
	CapPitchAndYaw();

	fmodf (m_yawSpeed, TRI_2PI );
	fmodf( m_yaw, TRI_2PI );

	// Find absolute camera position
	D3DXQuaternionRotationYawPitchRoll(&m_rotationAroundParent, m_yaw, m_pitch, 0.0f);
	Vector3 vecCamPos;
	TriVectorRotateQuaternion(&vecCamPos, &m_translationFromParent, &m_rotationAroundParent);

	vecCamPos += parentPosition;


	////////////////////////////////////////////////////////////////////////
	// Calculate perputal idle motion
	////////////////////////////////////////////////////////////////////////
	
	// Use the 'Eight' curve
	m_idleTheta += dT * m_idleSpeed;
	if(m_idleTheta > TRI_2PI)
	{
		m_idleTheta = fmodf(m_idleTheta, TRI_2PI);
	}

	float idleYaw=0.0f,idlePitch=0.0f;
	if(m_idleMove)
	{
		idleYaw = m_idleScale * cosf(m_idleTheta);
		idlePitch = 1.2f * idleYaw * sinf(m_idleTheta);
	}

	if(m_noiseCurve)
	{
		m_noise = m_noiseCurve->Update(t) > 0.0f;
	}
	if(m_noise)
	{
		if(m_noiseScaleCurve)
		{
			float noiseScale = m_noiseScaleCurve->Update(t);
			if( IsFinite( noiseScale ) )
			{
				m_noiseScale = noiseScale;
			}
			else
			{
				CCP_LOGERR( "EveCamera: Got non-finite noise scale." );
			}
		}
		if(m_noiseDampCurve)
		{
			float noiseDamp = m_noiseDampCurve->Update(t);
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
		m_NoisexN = (m_NoisexN + m_noiseDamp*ran)/(1.0f + m_noiseDamp*dT);
		m_NoisexN = Clamp( m_NoisexN, -m_maxNoise, m_maxNoise );
		ran = TriRand()-0.5f;
		m_NoiseyN = (m_NoiseyN + m_noiseDamp*ran)/(1.0f + m_noiseDamp*dT);
		m_NoiseyN = Clamp( m_NoiseyN, -m_maxNoise, m_maxNoise );

		idleYaw += m_noiseScale * m_NoisexN;
		idlePitch += m_noiseScale * m_NoiseyN;
	}
	
	// Find the interest point of the camera (probably the ship)
	Vector3 vecCamIntr(parentPosition);
	Vector3 toInterest = vecCamIntr - vecCamPos;
	Vector3 side = Vector3(1.0f,0.0f,0.0f);
	Vector3 up = Vector3(0.0f,1.0f,0.0f);
	
	D3DXVec3Normalize(&toInterest, &toInterest);
	//draw a line through the interest, 100 meters long
	Vector3 extendedInterest = toInterest * 100.0f;

	D3DXVec3Cross(&side, &toInterest, &up);
	D3DXVec3Cross(&up, &side, &toInterest);

	//all noise should be added to the interest point at 200 meters
	//so that the size of the interest won't affect the perceived noise (though it should)
	extendedInterest += idleYaw * side + idlePitch * up;

	// Add camera noise
	vecCamIntr = vecCamPos + extendedInterest;



	///////////////////////////////////////////////////////////////////////
	// And Finally animate the rotation around the 'other' interest if present
	////////////////////////////////////////////////////////////////////////
	m_yawInt = (m_yawInt + m_friction*dT*m_yawIntSpeed)/(1.0f + m_friction*dT);
	m_pitchInt = (m_pitchInt + m_friction*dT*m_pitchIntSpeed)/(1.0f + m_friction*dT);


	// cutoff
	if(fabs(m_yawInt-m_yawIntSpeed)< 0.0001f)
		m_yawInt = m_yawIntSpeed;

	if(fabs(m_pitchInt-m_pitchIntSpeed)< 0.0001f)
		m_pitchInt = m_pitchIntSpeed;

	// check if we are tracking an interest, in which
	// case determine its yaw and pitch and make that the target
	if(m_trackInterest && m_interestTranslationCurve)
	{
		Vector3 aPos;
		m_interestTranslationCurve->GetValueAt(&aPos, t);
		float aYaw,aPitch,r;
		aPos = aPos - vecCamPos;
		Vector3 tPos;

		tPos.x = D3DXVec3Dot(&aPos, &side);
		tPos.y = D3DXVec3Dot(&aPos, &up);
		tPos.z = D3DXVec3Dot(&aPos, &toInterest);
		r = D3DXVec3Length(&tPos);
		if(r>0.0f)
			aPitch = asinf(tPos.y/r);
		else
			aPitch = 0.0f;

		aYaw = atan2f(tPos.x,tPos.z);

		m_yawIntSpeed = -aYaw;
		m_pitchIntSpeed = aPitch;
	} 
	else if(m_rotationOfInterest.x == 0.0f && m_rotationOfInterest.y == 0.0f && m_rotationOfInterest.z == 0.0f && m_rotationOfInterest.w == 1.0f)
	{
		m_yawIntSpeed = m_pitchIntSpeed = 0.0f;
	}

	
	// Cap both yat and pitch
	if(m_pitchInt > m_maxPitch)
		m_pitchInt = m_maxPitch;
	else if (m_pitchInt < m_minPitch)
		m_pitchInt = m_minPitch;

	if(m_minYaw != m_maxYaw){
		if(m_yawInt > m_maxYaw)
			m_yawInt = m_maxYaw;
		else if (m_yaw < m_minYaw)
			m_yawInt = m_minYaw;
	}

	fmodf(m_yawIntSpeed, TRI_2PI);
	fmodf(m_yawInt, TRI_2PI);

	D3DXQuaternionRotationYawPitchRoll(&m_rotationOfInterest, m_yawInt, m_pitchInt, 0.0f);

	////////////////////////////////////////////////////////////////////////
	// Set up the camera at the calculated position and with the calculated interest
	//////////////////////////////////////////////////////////////////////// 
	
	// We transform the m_alignment direction to avoid having to worry about the view direction
	// and the alignemnt being parallel
	Vector3 realUpDir;
	TriVectorRotateQuaternion( &realUpDir, &m_alignment, &m_rotationAroundParent );
	D3DXVec3Normalize( &realUpDir, &realUpDir );

	if( !IsFinite( realUpDir ) || !D3DXVec3LengthSq( &realUpDir ) )
	{
		CCP_LOGERR( "EveCamera: Invalid up direction (%f, %f, %f), rotation (%f, %f, %f, %f)", 
			realUpDir.x, realUpDir.y, realUpDir.z,
			m_rotationAroundParent.x, m_rotationAroundParent.y, m_rotationAroundParent.z, m_rotationAroundParent.w);
		realUpDir = Vector3( 0, 1, 0 );
	}

	Vector3 lookAtLength;
	if( !D3DXVec3Length( D3DXVec3Subtract( &lookAtLength, &vecCamPos, &vecCamIntr ) ) )
	{
		CCP_LOGERR( "EveCamera: Camera position and camera interest identical(%f, %f, %f).",
			vecCamPos.x, vecCamPos.y, vecCamPos.z );
		vecCamPos += m_translationFromParent;
	}
	if( !IsFinite( vecCamIntr ) || !IsFinite( vecCamPos ) )
	{
		CCP_LOGERR( "EveCamera: Camera interest(%f, %f, %f) and/or position(%f, %f, %f) invalid.",
			vecCamIntr.x, vecCamIntr.y, vecCamIntr.z,
			vecCamPos.x, vecCamPos.y, vecCamPos.z);
		vecCamIntr = Vector3(0, 0, 0);
		vecCamPos = m_translationFromParent;
	}
	
	Matrix viewTransform, viewTransformInt;
	D3DXMatrixLookAtRH( &viewTransform, &vecCamPos, &vecCamIntr, &realUpDir);
	// now rotate toward other interest
	Quaternion tmpResult;
	TriMatrixRotate( &viewTransformInt, &viewTransform, static_cast<Quaternion*>( D3DXQuaternionInverse(&tmpResult, &m_rotationOfInterest) ) );

	CalculateProjectionMatrix( &m_projectionTransform, gTriDev->AspectRatio(), m_fieldOfView, 
		                       m_projectionCenterOffset, 0, m_frontClip, m_backClip, m_projectionMatrix );


	// update cached attribs
	m_pos = vecCamPos;
	m_intr = vecCamIntr;
	m_viewVec = Vector3( viewTransformInt._13, viewTransformInt._23, viewTransformInt._33 );
	m_upVec = Vector3( viewTransformInt._12, viewTransformInt._22, viewTransformInt._32 );
	m_rightVec = Vector3( viewTransformInt._11, viewTransformInt._21, viewTransformInt._31 );
	
	if( D3DXVec3Length( &m_viewVec ) && D3DXVec3Length( &m_upVec ) &&  D3DXVec3Length( &m_rightVec  ) )
	{
		m_viewMatrix->SetTransform(viewTransformInt);
	}
	else
	{
		CCP_LOGERR( "EveCamera: Invalid results after applying rotation of interest. Ignoring interest." );

		m_viewVec = Vector3( viewTransform._13, viewTransform._23, viewTransform._33 );
		m_upVec = Vector3( viewTransform._12, viewTransform._22, viewTransform._32 );
		m_rightVec = Vector3( viewTransform._11, viewTransform._21, viewTransform._31 );
		
		m_viewMatrix->SetTransform(viewTransform);
	}

	if( m_audio2Listener )
	{
		m_audio2Listener->UpdatePlacement( m_viewVec, m_upVec, m_pos );
	}
}


bool EveCamera::OnModified(
	Be::Var* value
	)
{
	if( IsMatch( value, m_rotationAroundParent ) )
	{	
		float crap;
		TriQuaternionToYawPitchRoll(&m_yaw, &m_pitch, &crap, &m_rotationAroundParent);
	} 
	else if( IsMatch( value, m_interestTranslationCurve ) )
	{
		if( m_interestTranslationCurve && (m_interestTranslationCurve != m_parentTranslationCurve))
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
	if(m_pitch > m_maxPitch)
		m_pitch = m_maxPitch;
	else if (m_pitch < m_minPitch)
		m_pitch = m_minPitch;

	if(m_minYaw != m_maxYaw){
		if(m_yaw > m_maxYaw)
			m_yaw = m_maxYaw;
		else if (m_yaw < m_minYaw)
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
	D3DXMatrixPerspectiveOffCenterRH( mat, left, right, bottom, top, front, back );

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
	float aspectRatio = ( mat._11 ? mat._22 / mat._11 : 0.0f);
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
	// see DirectX documentation for D3DXMatrixPerspectiveOffCenter
	float centerOffsetX = original._31;
	float centerOffsetY = original._32;

	EveCamera::CalculateProjectionMatrix( &projection, aspectRatio, fov, centerOffsetX, centerOffsetY, nearClip, farClip, NULL );

	return projection;
}