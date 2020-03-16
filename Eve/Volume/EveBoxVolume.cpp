#include "StdAfx.h"
#include "EveBoxVolume.h"
#include "ITr2Renderable.h"
#include "Tr2Renderer.h"
#include "IWorldPosition.h"
#include "Utilities/BoundingBox.h"


EveBoxVolume::EveBoxVolume( IRoot* lockobj ) :
	m_position( 0, 0, 0 ),
	m_centerOffset( 0, 0, 0 ),
	m_scaling( 0, 0, 0 ),
	m_innerScaling( 0, 0, 0 ),
	m_rotation( 0, 0, 0, 1 ),
	m_innerIntersection( 0, 0, 0 ),
	m_outerIntersection( 0, 0, 0 ),
	m_boxTransform( IdentityMatrix() ),
	m_innerBoxTransform( IdentityMatrix() ),
	m_rotationMatrix( IdentityMatrix() ),
	m_inverseRotation( IdentityMatrix() ),
	m_debugShowIntersection( false ),
	m_notifyParent( false )
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

void EveBoxVolume::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform )
{
	Vector3 min, max;
	max = Vector3( 0.5f, 0.5f, 0.5f );
	min = Vector3( -0.5f, -0.5f, -0.5f );

	if( LengthSq( m_scaling ) != 0 )
	{
		renderer.DrawBox( this, m_boxTransform * parentTransform, min, max, Tr2DebugRenderer::Wireframe, 0xff555555 );
	}
	if( LengthSq( m_innerScaling ) != 0 )
	{
		renderer.DrawBox( this, m_innerBoxTransform * parentTransform, min, max, Tr2DebugRenderer::Wireframe, 0xff777777 );
	}

	if( m_debugShowIntersection )
	{
		renderer.DrawSphere( this, m_rotationMatrix * parentTransform, m_innerIntersection, 1, 16, Tr2DebugRenderer::Solid, 0xffff0000 );
		renderer.DrawSphere( this, m_rotationMatrix * parentTransform, m_outerIntersection, 1, 16, Tr2DebugRenderer::Solid, 0xffffff00 );
	}
}

Vector4 EveBoxVolume::GetBoundingSphere() const
{
	return Vector4( m_position, Length( m_scaling ) * 0.5f );
}


float EveBoxVolume::GetIntensity( Vector3 position )
{

	Vector3 axisAlignedPosition = TransformCoord( position, m_inverseRotation );
	Vector3 rotatedOffset = TransformCoord( m_centerOffset, m_inverseRotation );

	Vector3 outerMin = m_position - m_scaling * 0.5;
	Vector3 outerMax = m_position + m_scaling * 0.5;

	Vector3 innerMin = m_position + rotatedOffset - m_innerScaling * 0.5;
	Vector3 innerMax = m_position + rotatedOffset + m_innerScaling * 0.5;


	// Are we outside the outer box?
	if( !BoundingBoxIsInside( outerMin, outerMax, axisAlignedPosition ) )
	{
		return 0.0f;
	}

	// Are we inside the inner box?
	if( BoundingBoxIsInside( innerMin, innerMax, axisAlignedPosition ) )
	{
		return 1.0f;
	}
	
	Vector3 rayDir = Normalize( m_position + rotatedOffset - axisAlignedPosition );

	// we are somewhere in between 
	IntersectAxisAlignedBoxRay( outerMin, outerMax, axisAlignedPosition, rayDir, m_outerIntersection );
	IntersectAxisAlignedBoxRay( innerMin, innerMax, axisAlignedPosition, rayDir, m_innerIntersection );

	return LengthSq( axisAlignedPosition - m_outerIntersection ) / LengthSq( m_innerIntersection - m_outerIntersection );
}


void EveBoxVolume::RegisterForChanges( std::function<void()> NotifyParent )
{
	m_notifyParentFunc = NotifyParent;
	m_notifyParent = true;
}

void EveBoxVolume::Setup()
{
	m_scaling = XMVectorMax( m_scaling, Vector3( 0, 0, 0 ) );
	m_innerScaling = XMVectorMin( XMVectorMax(m_innerScaling, Vector3( 0, 0, 0 ) ), m_scaling );

	m_boxTransform = TransformationMatrix( m_scaling, m_rotation, m_position );
	m_innerBoxTransform = TransformationMatrix( m_innerScaling, m_rotation, m_position + m_centerOffset );

	m_rotationMatrix = RotationMatrix( m_rotation );
	m_inverseRotation = Inverse( m_rotationMatrix );
}

//////////////////////////////////////////////////////////////////////////
// INotify
bool EveBoxVolume::OnModified( Be::Var* val )
{
	if(m_notifyParent && (IsMatch( val, m_position ) || IsMatch( val, m_scaling )))
	{
		Setup();

		m_notifyParentFunc();
	}
	if( IsMatch( val, m_rotation ) || IsMatch( val, m_innerScaling ) || IsMatch( val, m_centerOffset ) )
	{
		Setup();
	}
	return true;
}