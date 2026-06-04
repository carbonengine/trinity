//////////////////////////////////////////////////////////////////////////
//
// Created: March 2011
// Copyright CCP 2011
//

#include "StdAfx.h"

#include <cmath>

#include "Tr2ProjectBoundingBoxBracket.h"
#include "include/ITr2BoundingBox.h"
#include "Tr2Renderer.h"
#include "TriViewport.h"
#include "Sprite2d/Tr2Sprite2dContainer.h"
#include "Utilities/BoundingBox.h"
#include "include/ITr2DebugRenderer.h"

extern ITr2DebugRendererPtr g_debugRenderer;

namespace
{
	const float CLIP_EPSILON = 1e-5f;

	struct ClipPoint
	{
		float x;
		float y;
		float z;
		float w;
	};

	struct ProjectedBounds
	{
		float x;
		float y;
		float z;
		float width;
		float height;
		bool extendsOffscreen;
		bool coversViewport;
	};

	bool IsFinite( float value )
	{
		return std::isfinite( value );
	}

	bool IsFinite( const ClipPoint& point )
	{
		return IsFinite( point.x ) && IsFinite( point.y ) && IsFinite( point.z ) && IsFinite( point.w );
	}

	ClipPoint TransformPointToClip( const Vector3& point, const Matrix& viewProjection )
	{
		return ClipPoint
		{
			point.x * viewProjection._11 + point.y * viewProjection._21 + point.z * viewProjection._31 + viewProjection._41,
			point.x * viewProjection._12 + point.y * viewProjection._22 + point.z * viewProjection._32 + viewProjection._42,
			point.x * viewProjection._13 + point.y * viewProjection._23 + point.z * viewProjection._33 + viewProjection._43,
			point.x * viewProjection._14 + point.y * viewProjection._24 + point.z * viewProjection._34 + viewProjection._44
		};
	}

	ClipPoint Lerp( const ClipPoint& a, const ClipPoint& b, float t )
	{
		return ClipPoint
		{
			a.x + ( b.x - a.x ) * t,
			a.y + ( b.y - a.y ) * t,
			a.z + ( b.z - a.z ) * t,
			a.w + ( b.w - a.w ) * t
		};
	}

	bool AreAllOutsidePlane( const ClipPoint* points, float ( *planeDistance )( const ClipPoint& ) )
	{
		for( int i = 0; i < 8; ++i )
		{
			if( planeDistance( points[i] ) >= 0.0f )
			{
				return false;
			}
		}
		return true;
	}

	float DistanceToLeftPlane( const ClipPoint& point ) { return point.x + point.w; }
	float DistanceToRightPlane( const ClipPoint& point ) { return point.w - point.x; }
	float DistanceToBottomPlane( const ClipPoint& point ) { return point.y + point.w; }
	float DistanceToTopPlane( const ClipPoint& point ) { return point.w - point.y; }
	float DistanceToNearPlane( const ClipPoint& point ) { return point.z; }
	float DistanceToFarPlane( const ClipPoint& point ) { return point.w - point.z; }

	bool IsTriviallyOutsideFrustum( const ClipPoint* points )
	{
		return AreAllOutsidePlane( points, DistanceToLeftPlane ) ||
			AreAllOutsidePlane( points, DistanceToRightPlane ) ||
			AreAllOutsidePlane( points, DistanceToBottomPlane ) ||
			AreAllOutsidePlane( points, DistanceToTopPlane ) ||
			AreAllOutsidePlane( points, DistanceToNearPlane ) ||
			AreAllOutsidePlane( points, DistanceToFarPlane );
	}

	bool CanPerspectiveDivide( const ClipPoint& point )
	{
		return IsFinite( point ) && fabsf( point.w ) > CLIP_EPSILON;
	}

	void AddIfProjectable( const ClipPoint& point, std::vector<ClipPoint>& points )
	{
		if( point.z >= 0.0f && CanPerspectiveDivide( point ) )
		{
			points.push_back( point );
		}
	}

	void AddNearPlaneIntersection( const ClipPoint& a, const ClipPoint& b, std::vector<ClipPoint>& points )
	{
		if( ( a.z < 0.0f ) == ( b.z < 0.0f ) )
		{
			return;
		}

		float denominator = a.z - b.z;
		if( fabsf( denominator ) <= CLIP_EPSILON )
		{
			return;
		}

		float t = a.z / denominator;
		ClipPoint point = Lerp( a, b, t );
		if( CanPerspectiveDivide( point ) )
		{
			points.push_back( point );
		}
	}

	bool ProjectClipPoint( const ClipPoint& point, const TriViewport& viewport, Vector3& projected )
	{
		if( !CanPerspectiveDivide( point ) )
		{
			return false;
		}

		float reciprocalW = 1.0f / point.w;
		projected.x = viewport.x + ( 1.0f + point.x * reciprocalW ) * 0.5f * viewport.width;
		projected.y = viewport.y + ( 1.0f - point.y * reciprocalW ) * 0.5f * viewport.height;
		projected.z = viewport.minZ + point.z * reciprocalW * ( viewport.maxZ - viewport.minZ );
		return IsFinite( projected.x ) && IsFinite( projected.y ) && IsFinite( projected.z );
	}

	bool ProjectBoundingBoxToViewport( const Vector3& bbMin, const Vector3& bbMax, const Matrix& viewProjection, const TriViewport& viewport, ProjectedBounds& bounds )
	{
		Vector3 corners[8];
		corners[0] = bbMin;
		corners[1] = Vector3( bbMin.x, bbMin.y, bbMax.z );
		corners[2] = Vector3( bbMax.x, bbMin.y, bbMin.z );
		corners[3] = Vector3( bbMax.x, bbMin.y, bbMax.z );
		corners[4] = bbMax;
		corners[5] = Vector3( bbMax.x, bbMax.y, bbMin.z );
		corners[6] = Vector3( bbMin.x, bbMax.y, bbMax.z );
		corners[7] = Vector3( bbMin.x, bbMax.y, bbMin.z );

		ClipPoint clipCorners[8];
		for( int i = 0; i < 8; ++i )
		{
			clipCorners[i] = TransformPointToClip( corners[i], viewProjection );
			if( !IsFinite( clipCorners[i] ) )
			{
				return false;
			}
		}

		if( IsTriviallyOutsideFrustum( clipCorners ) )
		{
			return false;
		}

		std::vector<ClipPoint> projectablePoints;
		projectablePoints.reserve( 20 );
		for( int i = 0; i < 8; ++i )
		{
			AddIfProjectable( clipCorners[i], projectablePoints );
		}

		static const int EDGES[12][2] =
		{
			{ 0, 1 }, { 1, 3 }, { 3, 2 }, { 2, 0 },
			{ 7, 6 }, { 6, 4 }, { 4, 5 }, { 5, 7 },
			{ 0, 7 }, { 1, 6 }, { 2, 5 }, { 3, 4 }
		};

		for( int i = 0; i < 12; ++i )
		{
			AddNearPlaneIntersection( clipCorners[EDGES[i][0]], clipCorners[EDGES[i][1]], projectablePoints );
		}

		if( projectablePoints.empty() )
		{
			return false;
		}

		Vector3 projected;
		bool hasProjectedPoint = false;
		float minX = 0.0f;
		float minY = 0.0f;
		float minZ = 0.0f;
		float maxX = 0.0f;
		float maxY = 0.0f;

		for( const ClipPoint& point : projectablePoints )
		{
			if( !ProjectClipPoint( point, viewport, projected ) )
			{
				continue;
			}

			if( !hasProjectedPoint )
			{
				minX = maxX = projected.x;
				minY = maxY = projected.y;
				minZ = projected.z;
				hasProjectedPoint = true;
			}
			else
			{
				minX = std::min( minX, projected.x );
				maxX = std::max( maxX, projected.x );
				minY = std::min( minY, projected.y );
				maxY = std::max( maxY, projected.y );
				minZ = std::min( minZ, projected.z );
			}
		}

		if( !hasProjectedPoint )
		{
			return false;
		}

		float width = maxX - minX;
		float height = maxY - minY;
		if( !IsFinite( width ) || !IsFinite( height ) || width <= 0.0f || height <= 0.0f )
		{
			return false;
		}

		float viewportLeft = static_cast<float>( viewport.x );
		float viewportTop = static_cast<float>( viewport.y );
		float viewportRight = viewportLeft + static_cast<float>( viewport.width );
		float viewportBottom = viewportTop + static_cast<float>( viewport.height );

		bounds.x = minX;
		bounds.y = minY;
		bounds.z = minZ;
		bounds.width = width;
		bounds.height = height;
		bounds.extendsOffscreen = minX < viewportLeft || minY < viewportTop || maxX > viewportRight || maxY > viewportBottom;
		bounds.coversViewport = minX <= viewportLeft && minY <= viewportTop && maxX >= viewportRight && maxY >= viewportBottom;
		return true;
	}
}


Tr2ProjectBoundingBoxBracket::Tr2ProjectBoundingBoxBracket( IRoot* lockobj /*= NULL */ ) :
	m_minProjectedWidth( 0.0f ),
	m_minProjectedHeight( 0.0f ),
	m_maxProjectedWidth( 0.0f ),
	m_maxProjectedHeight( 0.0f ),
	m_projectedX( 0.0f ),
	m_projectedY( 0.0f ),
	m_projectedZ( 0.0f ),
	m_projectedWidth( 0.0f ),
	m_projectedHeight( 0.0f ),
	m_integerCoordinates( true ),
	m_screenMargin( 32.0f ),
	m_cameraDistance( 0 ),
	m_isProjectionValid( false ),
	m_containsCamera( false ),
	m_extendsOffscreen( false ),
	m_coversViewport( false )
{
}


void Tr2ProjectBoundingBoxBracket::UpdateValue( double time )
{
	if( !m_object )
	{
		SetEmptyProjection();
		return;
	}

	if( !m_object->IsBoundingBoxReady() )
	{
		SetEmptyProjection();
		return;
	}

	Vector3 bbMin, bbMax;
	if( !m_object->GetWorldBoundingBox( bbMin, bbMax ) )
	{
		SetEmptyProjection();
		return;
	}

	Vector3 center = (bbMax + bbMin) * 0.5f;
	Vector3 d = Tr2Renderer::GetViewPosition() - center;
	m_cameraDistance = Length( d );

	const TriViewport& viewport = Tr2Renderer::GetViewport();
	if( BoundingBoxIsInside( bbMin, bbMax, Tr2Renderer::GetViewPosition() ) )
	{
		m_projectedX = static_cast<float>( viewport.x );
		m_projectedY = static_cast<float>( viewport.y );
		m_projectedZ = viewport.minZ;
		m_projectedWidth = static_cast<float>( viewport.width );
		m_projectedHeight = static_cast<float>( viewport.height );
		m_isProjectionValid = true;
		m_containsCamera = true;
		m_extendsOffscreen = true;
		m_coversViewport = true;
		UpdateBracket();
		return;
	}

	Matrix viewProjection = Tr2Renderer::GetViewTransform() * Tr2Renderer::GetProjectionTransform();
	ProjectedBounds projectedBounds;
	if( !ProjectBoundingBoxToViewport( bbMin, bbMax, viewProjection, viewport, projectedBounds ) )
	{
		SetEmptyProjection();
		return;
	}

	m_projectedX = projectedBounds.x;
	m_projectedY = projectedBounds.y;
	m_projectedZ = projectedBounds.z;
	m_projectedWidth = projectedBounds.width;
	m_projectedHeight = projectedBounds.height;
	m_containsCamera = false;
	m_extendsOffscreen = projectedBounds.extendsOffscreen;
	m_coversViewport = projectedBounds.coversViewport;

	float centerX = m_projectedX + m_projectedWidth * 0.5f;
	float centerY = m_projectedY + m_projectedHeight * 0.5f;
	if( m_minProjectedWidth > 0.0f && m_projectedWidth < m_minProjectedWidth )
	{
		m_projectedWidth = m_minProjectedWidth;
	}
	else if( m_maxProjectedWidth > 0.0f && m_projectedWidth > m_maxProjectedWidth )
	{
		m_projectedWidth = m_maxProjectedWidth;
	}

	if( m_minProjectedHeight > 0.0f && m_projectedHeight < m_minProjectedHeight )
	{
		m_projectedHeight = m_minProjectedHeight;
	}
	else if( m_maxProjectedHeight > 0.0f && m_projectedHeight > m_maxProjectedHeight )
	{
		m_projectedHeight = m_maxProjectedHeight;
	}

	m_projectedX = centerX - m_projectedWidth * 0.5f;
	m_projectedY = centerY - m_projectedHeight * 0.5f;

	if( m_integerCoordinates )
	{
		m_projectedX = floor( m_projectedX + 0.5f );
		m_projectedY = floor( m_projectedY + 0.5f );
		m_projectedWidth = floor( m_projectedWidth + 0.5f );
		m_projectedHeight = floor( m_projectedHeight + 0.5f );
	}

	if( !IsFinite( m_projectedX ) || !IsFinite( m_projectedY ) || !IsFinite( m_projectedWidth ) || !IsFinite( m_projectedHeight ) || m_projectedWidth <= 0.0f || m_projectedHeight <= 0.0f )
	{
		SetEmptyProjection();
		return;
	}

	m_isProjectionValid = true;
	UpdateBracket();

	if( g_debugRenderer )
	{
		int x = (int)m_projectedX;
		int y = (int)m_projectedY;
		g_debugRenderer->Printf( x, y, 0xffffffff, "%S", m_name.c_str() );
		y += 16;
		g_debugRenderer->Printf( x, y, 0xffffffff, "(%5.2f, %5.2f)", m_projectedWidth, m_projectedHeight );
	}
}

void Tr2ProjectBoundingBoxBracket::SetEmptyProjection()
{
	m_projectedX = 0.0f;
	m_projectedY = 0.0f;
	m_projectedZ = 0.0f;
	m_projectedWidth = 0.0f;
	m_projectedHeight = 0.0f;
	m_isProjectionValid = false;
	m_containsCamera = false;
	m_extendsOffscreen = false;
	m_coversViewport = false;

	UpdateBracket();
}

void Tr2ProjectBoundingBoxBracket::UpdateBracket()
{
	if( m_bracket )
	{
		m_bracket->SetDisplayX( m_projectedX );
		m_bracket->SetDisplayY( m_projectedY );
		m_bracket->SetDisplayWidth( m_projectedWidth );
		m_bracket->SetDisplayHeight( m_projectedHeight );
	}

	if( m_bracketUpdateCallback )
	{
		m_bracketUpdateCallback.CallVoid( this );
	}
}
