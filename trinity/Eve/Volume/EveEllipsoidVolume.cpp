////////////////////////////////////////////////////////////
//
//    Created:   March 2020
//    Copyright: CCP 2020
//

#include "StdAfx.h"
#include "EveEllipsoidVolume.h"
#include "ITr2Renderable.h"
#include "Tr2Renderer.h"
#include "Utilities/BoundingSphere.h"

EveEllipsoidVolume::EveEllipsoidVolume( IRoot* lockobj ) :
	m_position( 0, 0, 0 ),
	m_shape( 0, 0, 0 ),
	m_innerShape( 0, 0, 0 ),
	m_innerIntersection( 0, 0, 0 ),
	m_outerIntersection( 0, 0, 0 ),
	m_rotation( 0, 0, 0, 1 ),
	m_rotationMatrix( IdentityMatrix() ),
	m_inverseRotationMatrix( IdentityMatrix() ),
	m_debugShowIntersection( false ),
	m_notifyParent( false )
{
}

EveEllipsoidVolume::~EveEllipsoidVolume()
{
}

bool EveEllipsoidVolume::Initialize()
{
	Setup();
	return true;
}

void EveEllipsoidVolume::Setup()
{
	// make sure the shape is positive
	m_shape = XMVectorMax( Vector3( 0, 0, 0 ), m_shape );
	// fit the inner shape inside of the outer shape and not negative
	m_innerShape = XMVectorMax( Vector3( 0, 0, 0 ), XMVectorMin( m_innerShape, m_shape ) );

	m_rotationMatrix = RotationMatrix( m_rotation );
	m_inverseRotationMatrix = Inverse( m_rotationMatrix );
}

void EveEllipsoidVolume::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform )
{
	Matrix outerTransform = TransformationMatrix( m_shape, m_rotation, m_position ) * parentTransform;
	renderer.DrawSphere( Tr2DebugObjectReference( this, 200 ), outerTransform, 20, Tr2DebugRenderer::Wireframe, 0xff4444ff );

	Matrix innerTransform = TransformationMatrix( m_innerShape, m_rotation, m_position ) * parentTransform;
	renderer.DrawSphere( Tr2DebugObjectReference( this, 200 ), innerTransform, 20, Tr2DebugRenderer::Wireframe, 0x4444ffff );

	if( m_debugShowIntersection )
	{
		renderer.DrawSphere( this, m_rotationMatrix * parentTransform, m_innerIntersection, 1, 15, Tr2DebugRenderer::Solid, 0xffff0000 );
		renderer.DrawSphere( this, m_rotationMatrix * parentTransform, m_outerIntersection, 1, 15, Tr2DebugRenderer::Solid, 0xffffff00 );
	}
}

Vector4 EveEllipsoidVolume::GetBoundingSphere() const
{
	return Vector4( m_position, max( m_shape.x, max( m_shape.y, m_shape.z ) ) );
}

float EveEllipsoidVolume::GetIntensity( Vector3 position )
{
	position = TransformCoord( position, m_inverseRotationMatrix );
	if( !IsPointInsideEllipsoid( m_position, m_shape, position ) )
	{
		return 0.0f;
	}
	if( IsPointInsideEllipsoid( m_position, m_innerShape, position ) )
	{
		return 1.0f;
	}

	m_innerIntersection = Vector3(0, 0, 0);

	Vector3 rayDir = Normalize( m_position - position );
	IntersectEllipsoidRayClosest( m_outerIntersection, m_position, m_shape, position, rayDir );
	if( LengthSq( m_innerShape ) != 0 )
	{
		IntersectEllipsoidRayClosest( m_innerIntersection, m_position, m_innerShape, position, rayDir );
	}
	return LengthSq( position - m_outerIntersection ) / LengthSq( m_innerIntersection - m_outerIntersection );
}

void EveEllipsoidVolume::RegisterForChanges( std::function<void()> NotifyParent )
{
	m_notifyParentFunc = NotifyParent;
	m_notifyParent = true;
}

//////////////////////////////////////////////////////////////////////////
// INotify
bool EveEllipsoidVolume::OnModified( Be::Var* val )
{
	if( m_notifyParent && ( IsMatch( val, m_position ) || IsMatch( val, m_shape ) ) )
	{
		Setup();
		m_notifyParentFunc();
	}
	if( IsMatch( val, m_innerShape ) || IsMatch( val, m_rotation ) )
	{
		Setup();
	}
	return true;
}