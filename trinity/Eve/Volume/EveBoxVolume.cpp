// Copyright © 2020 CCP ehf.

#include "StdAfx.h"
#include "EveBoxVolume.h"
#include "ITr2Renderable.h"
#include "Tr2Renderer.h"
#include "IWorldPosition.h"
#include "Utilities/BoundingBox.h"
#include "include/TriMath.h"

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
	m_debugShowIntersection( false ),
	m_nextCallbackID( 1 )
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
	renderer.DrawBox( this, m_innerBoxTransform * parentTransform, MIN_AABB, MAX_AABB, Tr2DebugRenderer::Wireframe, baseColor * 0.6f );

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

uint32_t EveBoxVolume::RegisterForChanges( const std::function<void()>& callBack )
{
	m_onChangeCallbacks[m_nextCallbackID] = callBack;
	return m_nextCallbackID++;
}

void EveBoxVolume::UnregisterForChanges( uint32_t callbackID )
{
	m_onChangeCallbacks.erase( callbackID );
}

void EveBoxVolume::Setup()
{
	m_scaling = XMVectorMax( m_scaling, Vector3( 0, 0, 0 ) );
	m_innerScaling = XMVectorMin( XMVectorMax( m_innerScaling, Vector3( 0, 0, 0 ) ), m_scaling );

	m_boxTransform = TransformationMatrix( m_scaling, m_rotation, m_position );
	m_innerBoxTransform = TransformationMatrix( m_innerScaling, m_rotation, m_position );

	m_inverseBoxTransform = Inverse( m_boxTransform );
	m_inverseInnerBoxTransform = Inverse( m_innerBoxTransform );

	m_boundingSphere.center = m_position;
	m_boundingSphere.radius = Length( m_scaling ) * 0.5f;
}


void EveBoxVolume::GeneratePointsInVolume( std::vector<Vector3>& points, size_t howManyToAdd, bool excludeInnerVolume, float fallOffFactor )
{
	if( m_scaling.x == 0.f || m_scaling.y == 0.f || m_scaling.z == 0.f )
	{
		// volume is not properly defined
		return;
	}

	float leftRightSideSize = ( m_scaling.x - m_innerScaling.x ) * m_scaling.y * m_scaling.z;
	float topBottomSize = m_innerScaling.x * ( m_scaling.y - m_innerScaling.y ) * m_scaling.z;
	float frontBackLidSize = m_innerScaling.x * m_innerScaling.y * ( m_scaling.z - m_innerScaling.z );

	float outerSidesSize = 2.f * ( leftRightSideSize + topBottomSize + frontBackLidSize );
	float innerToOuterSizeRatio = 0.f;

	if( !excludeInnerVolume )
	{
		float rangeX = m_scaling.x - m_innerScaling.x;
		float rangeY = m_scaling.y - m_innerScaling.y;
		float rangeZ = m_scaling.z - m_innerScaling.z;

		// as the outer volume is only half filled by default so we use the difference multiplied by 0.5
		float adjustedOuterCubeSize = ( m_innerScaling.x + 0.5f * rangeX ) * ( m_innerScaling.y + 0.5f * rangeY ) * ( m_innerScaling.z + 0.5f * rangeZ );
		innerToOuterSizeRatio = ( m_innerScaling.x * m_innerScaling.y * m_innerScaling.z ) / adjustedOuterCubeSize;
		innerToOuterSizeRatio = 1.f - pow( 1.f - innerToOuterSizeRatio, 0.8f + 0.2f * fallOffFactor ); // absorb more points into inner shape for steep falloffs
	}

	points.reserve( points.size() + howManyToAdd );
	Vector3 position;


	for( size_t i = 0; i < howManyToAdd; i++ )
	{
		float zonePicker = TriRand();

		if( zonePicker < innerToOuterSizeRatio )
		{
			// place point randomly in innerCube
			float X = m_innerScaling.x * TriRand() - 0.5f * m_innerScaling.x;
			float Y = m_innerScaling.y * TriRand() - 0.5f * m_innerScaling.y;
			float Z = m_innerScaling.z * TriRand() - 0.5f * m_innerScaling.z;
			position = Vector3( X, Y, Z );
			points.push_back( position );
			continue;
		}

		zonePicker *= outerSidesSize;

		if( zonePicker < 2.f * leftRightSideSize )
		{
			float rand = 0.5f * m_innerScaling.x + pow( TriRand(), fallOffFactor ) * ( m_scaling.x - m_innerScaling.x ) * 0.5f;
			float X = zonePicker < leftRightSideSize ? rand : -rand;
			float Y = TriRand() * m_scaling.y - 0.5f * m_scaling.y;
			float Z = TriRand() * m_scaling.z - 0.5f * m_scaling.z;
			position = Vector3( X, Y, Z );
		}
		else if( zonePicker < 2.f * ( leftRightSideSize + topBottomSize ) )
		{
			float rand = 0.5f * m_innerScaling.y + pow( TriRand(), fallOffFactor ) * ( m_scaling.y - m_innerScaling.y ) * 0.5f;
			float X = TriRand() * m_innerScaling.x - 0.5f * m_innerScaling.x;
			float Y = zonePicker < 2.f * leftRightSideSize + topBottomSize ? rand : -rand;
			float Z = TriRand() * m_scaling.z - 0.5f * m_scaling.z;
			position = Vector3( X, Y, Z );
		}
		else
		{
			float rand = 0.5f * m_innerScaling.y + pow( TriRand(), fallOffFactor ) * ( m_scaling.y - m_innerScaling.y ) * 0.5f;
			float X = TriRand() * m_innerScaling.x - 0.5f * m_innerScaling.x;
			float Y = TriRand() * m_innerScaling.y - 0.5f * m_innerScaling.y;
			float Z = zonePicker < outerSidesSize - frontBackLidSize ? rand : -rand;
			position = Vector3( X, Y, Z );
		}

		points.push_back( position );
	}
}

//////////////////////////////////////////////////////////////////////////
// INotify
bool EveBoxVolume::OnModified( Be::Var* val )
{
	if( ( IsMatch( val, m_position ) || IsMatch( val, m_scaling ) ) )
	{
		Setup();
	}

	if( IsMatch( val, m_rotation ) || IsMatch( val, m_innerScaling ) )
	{
		Setup();
	}

	for( auto callBack : m_onChangeCallbacks )
	{
		if( callBack.second != nullptr )
		{
			callBack.second();
		}
	}

	return true;
}
