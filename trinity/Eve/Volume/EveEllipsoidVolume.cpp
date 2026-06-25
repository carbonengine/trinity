// Copyright © 2020 CCP ehf.

#include "StdAfx.h"
#include "EveEllipsoidVolume.h"
#include "ITr2Renderable.h"
#include "Tr2Renderer.h"
#include "Utilities/BoundingSphere.h"
#include "include/TriMath.h"

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
	m_boundingSphere( Vector3( 0, 0, 0 ), 0 ),
	m_nextCallbackID( 1 )
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

	m_boundingSphere.center = m_position;
	m_boundingSphere.radius = max( m_shape.x, max( m_shape.y, m_shape.z ) );

	for( auto callBack : m_onChangeCallbacks )
	{
		if( callBack.second != nullptr )
		{
			callBack.second();
		}
	}
}

void EveEllipsoidVolume::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform, const Color& baseColor )
{
	Matrix outerTransform = TransformationMatrix( m_shape, m_rotation, m_position ) * parentTransform;
	renderer.DrawSphere( Tr2DebugObjectReference( this, 200 ), outerTransform, 20, Tr2DebugRenderer::Wireframe, baseColor * 0.5f );

	Matrix innerTransform = TransformationMatrix( m_innerShape, m_rotation, m_position ) * parentTransform;
	renderer.DrawSphere( Tr2DebugObjectReference( this, 200 ), innerTransform, 20, Tr2DebugRenderer::Wireframe, baseColor * 0.6f );

	if( m_debugShowIntersection )
	{
		renderer.DrawSphere( this, m_rotationMatrix * parentTransform, m_innerIntersection, 1, 15, Tr2DebugRenderer::Solid, 0xffff0000 );
		renderer.DrawSphere( this, m_rotationMatrix * parentTransform, m_outerIntersection, 1, 15, Tr2DebugRenderer::Solid, 0xffffff00 );
	}
}

const CcpMath::Sphere EveEllipsoidVolume::GetBoundingSphere() const
{
	return m_boundingSphere;
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

	m_innerIntersection = Vector3( 0, 0, 0 );

	Vector3 rayDir = Normalize( m_position - position );
	IntersectEllipsoidRayClosest( m_outerIntersection, m_position, m_shape, position, rayDir );
	if( LengthSq( m_innerShape ) != 0 )
	{
		IntersectEllipsoidRayClosest( m_innerIntersection, m_position, m_innerShape, position, rayDir );
	}
	return LengthSq( position - m_outerIntersection ) / LengthSq( m_innerIntersection - m_outerIntersection );
}

uint32_t EveEllipsoidVolume::RegisterForChanges( const std::function<void()>& callBack )
{
	m_onChangeCallbacks[m_nextCallbackID] = callBack;
	return m_nextCallbackID++;
}

void EveEllipsoidVolume::UnregisterForChanges( uint32_t callbackID )
{
	m_onChangeCallbacks.erase( callbackID );
}

void EveEllipsoidVolume::GeneratePointsInVolume( std::vector<Vector3>& points, size_t howManyToAdd, bool excludeInnerVolume, float fallOffFactor )
{
	Vector3 position;
	Quaternion rotation;
	points.reserve( points.size() + howManyToAdd );
	float sizeDifference = 0.f;

	if( !excludeInnerVolume )
	{
		sizeDifference = ( m_innerShape.x * m_innerShape.y * m_innerShape.z ) / ( m_shape.x * m_shape.y * m_shape.z );
		sizeDifference = 1.f - pow( 1.f - sizeDifference, 0.6f + 0.4f * fallOffFactor ); // absorb more points into innerSphere based on falloff
	}

	for( size_t i = 0; i < howManyToAdd; i++ )
	{
		float a = TRI_2PI * ( float( rand() ) / RAND_MAX );
		float z = ( float( rand() ) / RAND_MAX ) * 2.f - 1.f;
		Vector3 angle( sqrt( 1.f - z * z ) * cos( a ), sqrt( 1.f - z * z ) * sin( a ), z );
		angle = Normalize( angle );

		if( ( float( rand() ) / RAND_MAX ) > sizeDifference )
		{
			position = angle * ( m_innerShape + ( m_shape - m_innerShape ) * pow( (float)rand() / RAND_MAX, 0.75f * fallOffFactor ) );
		}
		else
		{
			position = angle * ( m_innerShape * pow( (float)rand() / RAND_MAX, 1.f / 3.f ) );
		}

		points.push_back( position );
	}

	return;
}

//////////////////////////////////////////////////////////////////////////
// INotify
bool EveEllipsoidVolume::OnModified( Be::Var* val )
{
	Setup();
	return true;
}