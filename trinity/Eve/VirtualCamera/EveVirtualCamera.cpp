// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveVirtualCamera.h"

#include "include/TriMath.h"
#include "TriView.h"
#include "TriProjection.h"
#include "EveVirtualCameraBehaviour.h"
#include "Eve/IEveSpaceObject2.h"
#include "Utilities/Vector3d.h"

namespace
{
static const Vector3 UP = Vector3( 0, 1, 0 );
static const Vector3 RIGHT = Vector3( 1, 0, 0 );
static const Vector3 FORWARD = Vector3( 0, 0, 1 );
static const float DEBUG_SCALAR = 0.01f;
static const float SCRUB_INCREMENT_DT = 1.0f / 60.0f;
static const int SCRUB_MAX_ITERATIONS = 20;
static const uint32_t DEBUG_COLOR_SUCCESS = 0xffaa9911;
static const uint32_t DEBUG_COLOR_FAIL = 0xffff0000;
static const Tr2DebugColor DEBUG_COLOR = Tr2DebugColor( DEBUG_COLOR_SUCCESS, DEBUG_COLOR_FAIL );
// Little buffer space between the camera position and the debug gizmos so they aren't visible when
// looking through the camera, regardless of near clipping plane.
static const float DEBUG_BORDER_SIZE = 0.1f;

Vector3 GetCenterOfAnchors( const PIEveSpaceObject2Vector& anchors )
{
	if( anchors.size() == 0 )
	{
		return Vector3( 0, 0, 0 );
	}
	else
	{
		Vector3d center = Vector3d( 0, 0, 0 ); // using doubles because precision.
		for( auto it = anchors.begin(); it != anchors.end(); ++it )
		{
			Vector3 out;
			( *it )->GetModelCenterWorldPosition( out );
			center += out;
		}
		center /= (double)anchors.size();
		return Vector3( (float)center.x, (float)center.y, (float)center.z );
	}
}

float GetAnchorsBoundingSphereRadius( const PIEveSpaceObject2Vector& anchors, const Vector3& center )
{
	if( anchors.size() == 0 )
	{
		// If behaviours are relative to the bounding sphere radius but we don't have one, might as well let them
		// work in kilometers. Space is pretty big after all.
		return 1000.0f;
	}
	else
	{
		float radius = 0.0f;
		for( auto it = anchors.begin(); it != anchors.end(); ++it )
		{
			Vector4 bs;
			if( ( *it )->GetBoundingSphere( bs ) )
			{
				Vector3 objCenter;
				( *it )->GetModelCenterWorldPosition( objCenter );
				float dist = Length( objCenter - center ) + bs.w;
				radius = std::max( dist, radius );
			}
		}
		if( radius == 0.0f )
		{
			return 1000.0f;
		}
		else
		{
			return radius;
		}
	}
}

Vector3 GetForwardDirectionOfAnchors( const PIEveSpaceObject2Vector& anchors )
{
	if( anchors.size() == 0 )
	{
		return Vector3( 0, 0, 1 );
	}
	else
	{
		Vector3d forward = Vector3d( 0, 0, 0 ); // using doubles because precision.
		for( auto it = anchors.begin(); it != anchors.end(); ++it )
		{
			Matrix out;
			( *it )->GetLocalToWorldTransform( out );
			Vector3 fwd;
			auto tmp = RotationQuaternion( out ); // apple-clang complains about "taking address of temporary"
			TriVectorRotateQuaternion( &fwd, &FORWARD, &tmp );
			forward += Normalize( fwd );
		}
		forward /= (double)anchors.size();
		return Normalize( Vector3( (float)forward.x, 0, (float)forward.z ) );
	}
}
}

EveVirtualCamera::EveVirtualCamera( IRoot* lockobj ) :
	PARENTLOCK( m_positionBehaviours ),
	PARENTLOCK( m_pointOfInterestBehaviours ),
	PARENTLOCK( m_fovBehaviours ),
	PARENTLOCK( m_rollBehaviours ),
	PARENTLOCK( m_positionAnchors ),
	PARENTLOCK( m_pointOfInterestAnchors ),
	m_name( "Virtual Camera" ),
	m_isRunning( false ),
	m_fov( 1.0f ),
	m_roll( 0.0f ),
	m_position( 0, 0, 0 ),
	m_pointOfInterest( 0, 0, 0 ),
	m_localElapsedTime( 0.0f ),
	m_animationTimelineLength( 10.0f ),
	m_projection(),
	m_positionAnchorCenter(),
	m_positionAnchorRadius(),
	m_positionAnchorForwardDirection(),
	m_pointOfInterestAnchorCenter(),
	m_pointOfInterestAnchorRadius(),
	m_pointOfInterestAnchorForwardDirection()
{
	m_projection.CreateInstance();
}

EveVirtualCamera::~EveVirtualCamera()
{
}

Matrix EveVirtualCamera::GetViewMatrix()
{
	return LookAtMatrix( m_position, m_pointOfInterest, GetUpDirection() );
}

Matrix EveVirtualCamera::GetProjectionMatrix( float aspectRatio, float frontClip, float backClip )
{
	return PerspectiveFovMatrix( m_fov, aspectRatio, frontClip, backClip );
}

Vector3 EveVirtualCamera::GetViewDirection() const
{
	return Normalize( m_pointOfInterest - m_position );
}

Vector3 EveVirtualCamera::GetForwardDirection() const
{
	return GetViewDirection();
}

Vector3 EveVirtualCamera::GetUpDirection() const
{
	Quaternion rot;
	Vector3 viewDir = GetForwardDirection();
	Vector3 right = Normalize( Cross( viewDir, UP ) );
	Vector3 up = Normalize( Cross( right, viewDir ) );

	Quaternion roll = RotationQuaternion( viewDir, XMConvertToRadians( -m_roll ) );
	TriVectorRotateQuaternion( &up, &up, &roll );
	return Normalize( up );
}

Vector3 EveVirtualCamera::GetRightDirection() const
{
	auto viewDir = GetForwardDirection();
	auto up = GetUpDirection();
	return Normalize( Cross( viewDir, up ) );
}

void EveVirtualCamera::Update( float deltaTime )
{
	if( !m_isRunning )
	{
		deltaTime = 0.0f;
	}

	m_localElapsedTime += deltaTime;

	m_positionAnchorCenter = GetCenterOfAnchors( m_positionAnchors );
	m_positionAnchorForwardDirection = GetForwardDirectionOfAnchors( m_positionAnchors );
	m_positionAnchorRadius = GetAnchorsBoundingSphereRadius( m_positionAnchors, m_positionAnchorCenter );

	m_pointOfInterestAnchorCenter = GetCenterOfAnchors( m_pointOfInterestAnchors );
	m_pointOfInterestAnchorForwardDirection = GetForwardDirectionOfAnchors( m_pointOfInterestAnchors );
	m_pointOfInterestAnchorRadius = GetAnchorsBoundingSphereRadius( m_pointOfInterestAnchors, m_pointOfInterestAnchorCenter );

	auto position = m_positionAnchorCenter;
	auto pointOfInterest = m_pointOfInterestAnchorCenter;
	auto fov = 1.0f;
	auto roll = 0.0f;

	for( auto it = m_positionBehaviours.begin(); it != m_positionBehaviours.end(); ++it )
	{
		if( ( *it )->IsActive() )
		{
			position += ( *it )->Update( *this, position, deltaTime, m_localElapsedTime, m_positionAnchorCenter, m_positionAnchorRadius, m_positionAnchorForwardDirection );
		}
	}

	for( auto it = m_pointOfInterestBehaviours.begin(); it != m_pointOfInterestBehaviours.end(); ++it )
	{
		if( ( *it )->IsActive() )
		{
			pointOfInterest += ( *it )->Update( *this, pointOfInterest, deltaTime, m_localElapsedTime, m_pointOfInterestAnchorCenter, m_pointOfInterestAnchorRadius, m_pointOfInterestAnchorForwardDirection );
		}
	}

	for( auto it = m_fovBehaviours.begin(); it != m_fovBehaviours.end(); ++it )
	{
		if( ( *it )->IsActive() )
		{
			fov += ( *it )->Update( *this, fov, deltaTime, m_localElapsedTime, m_positionAnchorCenter, m_positionAnchorRadius, m_positionAnchorForwardDirection );
		}
	}

	for( auto it = m_rollBehaviours.begin(); it != m_rollBehaviours.end(); ++it )
	{
		if( ( *it )->IsActive() )
		{
			roll += ( *it )->Update( *this, roll, deltaTime, m_localElapsedTime, m_positionAnchorCenter, m_positionAnchorRadius, m_positionAnchorForwardDirection );
		}
	}

	if( m_positionBehaviours.size() > 0 )
	{
		m_position = position;
	}
	if( m_pointOfInterestBehaviours.size() > 0 )
	{
		m_pointOfInterest = pointOfInterest;
	}
	if( m_fovBehaviours.size() > 0 )
	{
		m_fov = fov;
	}
	if( m_rollBehaviours.size() > 0 )
	{
		m_roll = roll;
	}
}

void EveVirtualCamera::Play()
{
	if( m_isRunning )
	{
		return;
	}

	m_isRunning = true;
}

void EveVirtualCamera::Pause()
{
	if( !m_isRunning )
	{
		return;
	}

	m_isRunning = false;
}

void EveVirtualCamera::Stop()
{
	Reset();
	if( !m_isRunning )
	{
		return;
	}

	m_isRunning = false;
}

void EveVirtualCamera::Reset()
{
	m_localElapsedTime = 0.0;
}

void EveVirtualCamera::UpdateToLocalTime( float time )
{
	auto diff = time - m_localElapsedTime;
	auto dt = SCRUB_INCREMENT_DT;
	int iterations = static_cast<int>( floor( abs( diff / dt ) ) );
	if( iterations > SCRUB_MAX_ITERATIONS )
	{
		iterations = SCRUB_MAX_ITERATIONS;
		dt = diff / SCRUB_MAX_ITERATIONS;
	}
	iterations -= 1;
	auto wasRunning = m_isRunning;
	Play();
	for( int i = 0; i < iterations; ++i )
	{
		Update( dt );
	}
	Update( time - m_localElapsedTime );
	if( !wasRunning )
	{
		Pause();
	}
}

void EveVirtualCamera::CopyTransform( const EveVirtualCameraPtr& source )
{
	m_fov = source->m_fov;
	m_roll = source->m_roll;
	m_position = source->m_position;
	m_pointOfInterest = source->m_pointOfInterest;
}

void EveVirtualCamera::UpdateExternal( Vector3 position, Vector3 pointOfInterest, float fov, float roll )
{
	m_position = position;
	m_pointOfInterest = pointOfInterest;
	m_fov = fov;
	m_roll = roll;
}

const std::string& EveVirtualCamera::GetName() const
{
	return m_name;
}

void EveVirtualCamera::SetName( const std::string& name )
{
	m_name = name;
}

float EveVirtualCamera::GetAnimationTimelineLength() const
{
	return m_animationTimelineLength;
}

void EveVirtualCamera::SetAnimationTimelineLength( float value )
{
	m_animationTimelineLength = value;
}

float EveVirtualCamera::GetFov() const
{
	return m_fov;
}

void EveVirtualCamera::SetFov( float fov )
{
	m_fov = fov;
}

float EveVirtualCamera::GetRoll() const
{
	return m_roll;
}

void EveVirtualCamera::SetRoll( float roll )
{
	m_roll = roll;
}

Vector3 EveVirtualCamera::GetPosition() const
{
	return m_position;
}

void EveVirtualCamera::SetPosition( const Vector3& position )
{
	m_position = position;
}

Vector3 EveVirtualCamera::GetPointOfInterest() const
{
	return m_pointOfInterest;
}

void EveVirtualCamera::SetPointOfInterest( const Vector3& pointOfInterest )
{
	m_pointOfInterest = pointOfInterest;
}

void EveVirtualCamera::AddPositionBehaviour( EveVirtualCameraBehaviourVector3Base* behaviour )
{
	m_positionBehaviours.Append( behaviour );
}

void EveVirtualCamera::AddPointOfInterestBehaviour( EveVirtualCameraBehaviourVector3Base* behaviour )
{
	m_pointOfInterestBehaviours.Append( behaviour );
}

void EveVirtualCamera::AddFOVBehaviour( EveVirtualCameraBehaviourFloatBase* behaviour )
{
	m_fovBehaviours.Append( behaviour );
}

void EveVirtualCamera::AddRollBehaviour( EveVirtualCameraBehaviourFloatBase* behaviour )
{
	m_rollBehaviours.Append( behaviour );
}

void EveVirtualCamera::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Virtual Cameras" );
}

void EveVirtualCamera::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( renderer.HasOption( this, "Virtual Cameras" ) )
	{
		m_positionAnchorRadius = GetAnchorsBoundingSphereRadius( m_positionAnchors, m_positionAnchorCenter );
		m_pointOfInterestAnchorRadius = GetAnchorsBoundingSphereRadius( m_pointOfInterestAnchors, m_pointOfInterestAnchorCenter );

		auto coneSize = std::max( m_positionAnchorRadius * DEBUG_SCALAR, 1.0f );
		auto poiSize = std::max( m_pointOfInterestAnchorRadius * DEBUG_SCALAR, 1.0f );

		auto invView = Inverse( GetViewMatrix() );
		auto coneTransform = TransformationMatrix( Vector3( 1, 1, 1 ), RotationQuaternion( RIGHT, XM_PI / 2 ), Vector3( 0, 0, coneSize / 2.0f + DEBUG_BORDER_SIZE ) ) * invView;
		auto filmReel1Transform = TransformationMatrix( Vector3( 1, 1, 1 ), RotationQuaternion( FORWARD, XM_PI / 2 ), Vector3( 0, coneSize * 2.0f, coneSize * 2.0f ) ) * invView;
		auto filmReel2Transform = TransformationMatrix( Vector3( 1, 1, 1 ), RotationQuaternion( FORWARD, XM_PI / 2 ), Vector3( 0, coneSize * 1.675f, coneSize * 3.55f ) ) * invView;

		// Draw the camera
		renderer.DrawCylinder( this, filmReel1Transform, coneSize, coneSize / 4, 12, Tr2DebugRenderer::Lit, DEBUG_COLOR );
		renderer.DrawCylinder( this, filmReel2Transform, coneSize / 1.5f, coneSize / 4, 12, Tr2DebugRenderer::Lit, DEBUG_COLOR );
		renderer.DrawCone( this, coneTransform, -coneSize, coneSize, 12, Tr2DebugRenderer::Lit, DEBUG_COLOR );
		renderer.DrawBox( this, invView, Vector3( coneSize / 1.5f, coneSize, coneSize + DEBUG_BORDER_SIZE ), Vector3( -coneSize / 1.5f, -coneSize, coneSize * 4 ), Tr2DebugRenderer::Lit, DEBUG_COLOR );
		renderer.DrawText( TRI_DBG_FONT_LARGE, m_position + GetForwardDirection() * coneSize * -2.0f, DEBUG_COLOR_SUCCESS, m_name.c_str() );

		// Draw the point of interest
		renderer.DrawText( TRI_DBG_FONT_LARGE, m_pointOfInterest, DEBUG_COLOR_SUCCESS, ( m_name + " POI" ).c_str() );
		renderer.DrawSphere( this, m_pointOfInterest, poiSize, 16, Tr2DebugRenderer::Lit, DEBUG_COLOR );
	}
}
