////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"
#include "Tr2FollowCurveKey.h"
#include "IWorldPosition.h"
#include "include/TriMath.h"


Tr2ObjectFollowCurveKey::Tr2ObjectFollowCurveKey( IRoot* lockobj ) : 
	m_time( 0 ),
	m_leftTangent( 0, 0, 0 ),
	m_rightTangent( 0, 0, 0 ),
	m_rotatedLeftTangent( 0, 0, 0 ),
	m_rotatedRightTangent( 0, 0, 0 ),
	m_interpolation( Tr2FollowCurveKeyInterpolation::LINEAR ),
	m_offset( 0.0, 0.0, 0.0 ),
	m_rotationSetting( NO_ROTATION ),
	m_locator(nullptr)
{
}

Tr2ObjectFollowCurveKey::~Tr2ObjectFollowCurveKey()
{
	m_object = nullptr;
	m_locator = nullptr;
}

const float Tr2ObjectFollowCurveKey::GetTime()
{
	return m_time;
}

const Vector3 Tr2ObjectFollowCurveKey::GetLeftTangent()
{
	return m_rotatedLeftTangent;
}

const Vector3 Tr2ObjectFollowCurveKey::GetRightTangent()
{
	return m_rotatedRightTangent;
}

const Tr2FollowCurveKeyInterpolation::Type Tr2ObjectFollowCurveKey::GetInterpolationType()
{
	return m_interpolation;
}

bool Tr2ObjectFollowCurveKey::Initialize()
{
	m_locator = GetLocator();
	return true;
}

bool Tr2ObjectFollowCurveKey::OnModified( Be::Var *value )
{
	if( IsMatch( value, m_offsetLocatorName ) || IsMatch( value, m_object ) )
	{
		m_locator = GetLocator();
	}
	return true;
}


Locator* Tr2ObjectFollowCurveKey::GetLocator()
{
	if( m_object == nullptr || m_offsetLocatorName.empty() )
	{
		return nullptr;
	}

	if( EveSpaceObject2Ptr so = BlueCastPtr( m_object ) )
	{
		auto locators = so->GetLocatorsForSet( m_offsetLocatorName );
		if( locators != nullptr && locators->size() > 0 )
		{
			return const_cast< Locator* >( &( *locators )[0] );
		}
	}
	return nullptr;
}

Matrix Tr2ObjectFollowCurveKey::GetLocatorRotation()
{
	if( m_locator != nullptr )
	{
		return RotationMatrix( Normalize( m_locator->direction ) );
	}
	return Matrix();
}

Matrix Tr2ObjectFollowCurveKey::GetModelRotation()
{
	if( IWorldPositionPtr wp = BlueCastPtr( m_object ) )
	{
		auto q = Normalize( wp->GetWorldRotation() );
		return RotationMatrix( q );
	}
	return Matrix();
}

Vector3 Tr2ObjectFollowCurveKey::GetValue()
{
	if( m_object == nullptr )
	{
		return m_offset;
	}

	Vector3 offset = m_offset;
	if( m_locator != nullptr )
	{
		offset += m_locator->position;
	}

	Matrix rotationMatrix;
	switch( m_rotationSetting )
	{
	case LOCATOR_ROTATION:
		rotationMatrix = GetLocatorRotation();
		break;
	case MODEL_ROTATION:
		rotationMatrix = GetModelRotation();
		break;
	case NO_ROTATION:
	default:
		rotationMatrix = Matrix();
		break;
	}

	m_rotatedLeftTangent = TransformCoord( m_leftTangent, rotationMatrix );
	m_rotatedRightTangent = TransformCoord( m_rightTangent, rotationMatrix );

	// rotate the offset
	offset = TransformCoord( offset, rotationMatrix );

	if( IWorldPositionPtr wp = BlueCastPtr( m_object ) )
	{
		return wp->GetWorldPosition() + offset;
	}

	return offset;
}


Tr2CameraFollowCurveKey::Tr2CameraFollowCurveKey( IRoot* lockobj ) :
	m_time( 0 ),
	m_leftTangent( 0, 0, 0 ),
	m_rightTangent( 0, 0, 0 ),
	m_rotatedLeftTangent( 0, 0, 0 ),
	m_rotatedRightTangent( 0, 0, 0 ),
	m_interpolation( Tr2FollowCurveKeyInterpolation::LINEAR ),
	m_objectBounds( 0, 0, 0 ),
	m_offset( 0, 0, 0 ),
	m_fovMultiplication( 0.5f ),
	m_angle( 0.0f ),
	m_angleZero( TRI_PIBY2 ),
	m_boxPosition( 0, 0, 0 ),
	m_frontClip( 0 ),
	m_fov( 0 ),
	m_minDistanceAlongViewAngle( 0 ),
	m_minDistanceFromViewAngle( 0 ),
	m_enabled( true ),
	m_lastEnabledFOV( 0 ),
	m_lastEnabledFrontClip( 10 ),
	m_lastEnabledInverseViewMatrix( Matrix() )
{

}

Tr2CameraFollowCurveKey::~Tr2CameraFollowCurveKey()
{

}

bool Tr2CameraFollowCurveKey::Initialize()
{
	CalculateBoxPosition();
	return true;
}

bool Tr2CameraFollowCurveKey::OnModified( Be::Var *value )
{
	if( IsMatch( value, m_fovMultiplication ) )
	{
		// need to limit the fov multiplication so we can fit the box somewhere
		m_fovMultiplication = min( 0.999f, max( 0.001f, m_fovMultiplication ) );
	}
	if( IsMatch( value, m_enabled ) )
	{
		m_lastEnabledFOV = Tr2Renderer::GetFieldOfView() * 0.5f;
		m_lastEnabledInverseViewMatrix = Tr2Renderer::GetInverseViewTransform();
		m_lastEnabledFrontClip = Tr2Renderer::GetFrontClip();
	}
	return true;
}

/*
	This function finds the place between two cones where xy object bounds can be placed. 
	It sets two variables:
	- one how far down the view vector the front of the bounds are offset
	- one how far away from the view vector the bounds are offset

	Then it calculates the box position based on those variables
*/
void Tr2CameraFollowCurveKey::CalculateBoxPosition()
{
	auto rotMatrix = Tr2Renderer::GetInverseViewTransform();
	m_fov = Tr2Renderer::GetFieldOfView() * 0.5f;
	m_frontClip = Tr2Renderer::GetFrontClip();

	if( !m_enabled )
	{
		rotMatrix = m_lastEnabledInverseViewMatrix;
		m_fov = m_lastEnabledFOV;
		m_frontClip = m_lastEnabledFrontClip;
	}

	
	float tanOuterFov = tan( m_fov );
	float tanInnerFov = tan( m_fov * m_fovMultiplication );
		
	float nearClipPlaneSize = m_frontClip / tanOuterFov;
	float aspectRatio = Tr2Renderer::GetAspectRatio();

	float nearClipDistFromViewVector = max( nearClipPlaneSize * aspectRatio, nearClipPlaneSize / aspectRatio );
	float radius = max( Length( m_objectBounds.GetXY() ), nearClipDistFromViewVector );

	if( tanInnerFov == tanOuterFov )
	{
		m_minDistanceFromViewAngle = 0;
		m_minDistanceAlongViewAngle = 0;
		return;
	}

	m_minDistanceAlongViewAngle = 2.0f * radius / ( tanOuterFov - tanInnerFov );
	m_minDistanceFromViewAngle = m_minDistanceAlongViewAngle * tanInnerFov;
	if( m_minDistanceAlongViewAngle < m_frontClip + m_objectBounds.z )
	{
		m_minDistanceAlongViewAngle = m_frontClip + m_objectBounds.z;
		m_minDistanceFromViewAngle = m_minDistanceAlongViewAngle * tanInnerFov;
	}

	auto boxCenter = Vector3( 0, 0, -m_minDistanceAlongViewAngle - m_objectBounds.z ) + m_offset * Vector3( 1, 1, -1 );
	auto orientationMatrix = RotationMatrix( RotationQuaternion( Vector3( 0, 0, -1 ), m_angle + m_angleZero ) );

	auto boxOffsetXY = TransformCoord( Vector3( radius + m_minDistanceFromViewAngle, 0, 0 ), orientationMatrix );

	m_rotatedLeftTangent = TransformCoord( m_leftTangent, rotMatrix );
	m_rotatedRightTangent = TransformCoord( m_rightTangent, rotMatrix );
	m_boxPosition = TransformCoord( boxOffsetXY + boxCenter, rotMatrix );
}

Vector3 Tr2CameraFollowCurveKey::GetValue( )
{
	CalculateBoxPosition();
	
	return m_boxPosition;
}

const float Tr2CameraFollowCurveKey::GetTime()
{
	return m_time;
}

const Vector3 Tr2CameraFollowCurveKey::GetLeftTangent()
{
	return m_rotatedLeftTangent;
}

const Vector3 Tr2CameraFollowCurveKey::GetRightTangent()
{
	return m_rotatedRightTangent;
}

const Tr2FollowCurveKeyInterpolation::Type Tr2CameraFollowCurveKey::GetInterpolationType()
{
	return m_interpolation;
}