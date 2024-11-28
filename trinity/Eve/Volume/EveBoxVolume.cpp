////////////////////////////////////////////////////////////
//
//    Created:   March 2020
//    Copyright: CCP 2020
//

#include "StdAfx.h"
#include "EveBoxVolume.h"
#include "ITr2Renderable.h"
#include "Tr2Renderer.h"
#include "IWorldPosition.h"
#include "Utilities/BoundingBox.h"

Vector3 const EveBoxVolume::MAX_AABB = Vector3( 0.5, 0.5, 0.5 );
Vector3 const EveBoxVolume::MIN_AABB = Vector3( -0.5, -0.5, -0.5 );

EveBoxVolume::EveBoxVolume( IRoot* lockobj ) :
	m_position( 0, 0, 0 ),
	m_scaling( 0, 0, 0 ),
	m_innerScaling( 0, 0, 0 ),
	m_rotation( 0, 0, 0, 1 ),
	m_innerIntersection( 0, 0, 0 ),
	m_outerIntersection( 0, 0, 0 ),
	m_boxTransform( IdentityMatrix() ),
	m_innerBoxTransform( IdentityMatrix() ),
	m_inverseBoxTransform( IdentityMatrix() ),
	m_inverseInnerBoxTransform( IdentityMatrix() ),
	m_boundingSphere( Vector3( 0, 0, 0 ), 0 ),
	m_debugShowIntersection( false )
{
}

EveBoxVolume::~EveBoxVolume()
{
}

bool EveBoxVolume::Initialize()
{
	Setup();
	return true;
}

void EveBoxVolume::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform, const Color& baseColor )
{
	renderer.DrawBox( this, m_boxTransform * parentTransform, MIN_AABB, MAX_AABB, Tr2DebugRenderer::Wireframe, baseColor * 0.5f );
	renderer.DrawBox( this, m_innerBoxTransform * parentTransform, MIN_AABB, MAX_AABB, Tr2DebugRenderer::Wireframe, baseColor * 0.75f );

	if( m_debugShowIntersection )
	{
		renderer.DrawSphere( this, parentTransform, TransformCoord( m_innerIntersection, m_boxTransform ), 1, 16, Tr2DebugRenderer::Solid, 0xff555555 );
		renderer.DrawSphere( this, parentTransform, TransformCoord( m_outerIntersection, m_boxTransform ), 1, 16, Tr2DebugRenderer::Solid, 0xffffff00 );
	}
}

const CcpMath::Sphere EveBoxVolume::GetBoundingSphere() const
{
	return m_boundingSphere;
}


float EveBoxVolume::GetIntensity( Vector3 position )
{
	Vector3 axisAlignedPosition = TransformCoord( position, m_inverseBoxTransform );

	// Are we outside the outer box?
	if( !BoundingBoxIsInside( MIN_AABB, MAX_AABB, axisAlignedPosition ) )
	{
		return 0.0f;
	}

	Vector3 axisAlignedInnerPosition = TransformCoord( position, m_inverseInnerBoxTransform );

	// Are we inside the inner box?
	if( BoundingBoxIsInside( MIN_AABB, MAX_AABB, axisAlignedInnerPosition ) )
	{
		return 1.0f;
	}
	 
	Vector3 rayDir = Normalize( -axisAlignedPosition );

	// we are somewhere in between 
	IntersectAxisAlignedBoxRay( MIN_AABB, MAX_AABB, axisAlignedPosition, rayDir, m_outerIntersection );
	IntersectAxisAlignedBoxRay( MIN_AABB, MAX_AABB, axisAlignedInnerPosition, rayDir, m_innerIntersection );

	// move the inner intersection to the outer box space
	m_innerIntersection = TransformCoord( m_innerIntersection, m_innerBoxTransform * m_inverseBoxTransform );

	return LengthSq( axisAlignedPosition - m_outerIntersection ) / LengthSq( m_innerIntersection - m_outerIntersection );
}

void EveBoxVolume::Setup()
{
	m_scaling = XMVectorMax( m_scaling, Vector3( 0, 0, 0 ) );
	m_innerScaling = XMVectorMin( XMVectorMax(m_innerScaling, Vector3( 0, 0, 0 ) ), m_scaling );

	m_boxTransform = TransformationMatrix( m_scaling, m_rotation, m_position );
	m_innerBoxTransform = TransformationMatrix( m_innerScaling, m_rotation, m_position );

	m_inverseBoxTransform = Inverse( m_boxTransform );
	m_inverseInnerBoxTransform = Inverse( m_innerBoxTransform );

	m_boundingSphere.center = m_position;
	m_boundingSphere.radius = Length( m_scaling ) * 0.5f;
}

//////////////////////////////////////////////////////////////////////////
// INotify
bool EveBoxVolume::OnModified( Be::Var* val )
{
	Setup();
	return true;
}