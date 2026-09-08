// Copyright © 2011 CCP ehf.

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

Vector4 TransformPointToClip( const Vector3& point, const Matrix& viewProjection )
{
	return Vector4{
		point.x * viewProjection._11 + point.y * viewProjection._21 + point.z * viewProjection._31 + viewProjection._41,
		point.x * viewProjection._12 + point.y * viewProjection._22 + point.z * viewProjection._32 + viewProjection._42,
		point.x * viewProjection._13 + point.y * viewProjection._23 + point.z * viewProjection._33 + viewProjection._43,
		point.x * viewProjection._14 + point.y * viewProjection._24 + point.z * viewProjection._34 + viewProjection._44
	};
}

Vector4 Lerp( const Vector4& a, const Vector4& b, float t )
{
	return Vector4{
		a.x + ( b.x - a.x ) * t,
		a.y + ( b.y - a.y ) * t,
		a.z + ( b.z - a.z ) * t,
		a.w + ( b.w - a.w ) * t
	};
}

// Cohen-Sutherland style outcodes: one bit per plane of the D3D clip volume
// (-w <= x <= w, -w <= y <= w, 0 <= z <= w); a set bit means the point is
// outside that plane. In 2D (4 bits) the zones look like:
//
//          |        |
//     1001 |  1000  | 1010
//    ------+--------+------
//     0001 |  0000  | 0010   <- 0000 = inside
//    ------+--------+------
//     0101 |  0100  | 0110
//
// AND of all corner codes != 0 => every corner shares an outside plane
// => the box is fully off-frustum.
// XOR of two corner codes & CLIP_NEAR => the edge crosses the near plane.
enum ClipPlaneBits : uint32_t
{
	CLIP_LEFT = 1 << 0,
	CLIP_RIGHT = 1 << 1,
	CLIP_BOTTOM = 1 << 2,
	CLIP_TOP = 1 << 3,
	CLIP_NEAR = 1 << 4,
	CLIP_FAR = 1 << 5,
};

uint32_t ClipOutcode( const Vector4& point )
{
	uint32_t code = 0;
	if( point.x + point.w < 0.0f )
	{
		code |= CLIP_LEFT;
	}
	if( point.w - point.x < 0.0f )
	{
		code |= CLIP_RIGHT;
	}
	if( point.y + point.w < 0.0f )
	{
		code |= CLIP_BOTTOM;
	}
	if( point.w - point.y < 0.0f )
	{
		code |= CLIP_TOP;
	}
	if( point.z < 0.0f )
	{
		code |= CLIP_NEAR;
	}
	if( point.w - point.z < 0.0f )
	{
		code |= CLIP_FAR;
	}
	return code;
}

bool CanPerspectiveDivide( const Vector4& point )
{
	return fabsf( point.w ) > CLIP_EPSILON;
}

// Both endpoints must be on opposite sides of the near plane; the caller
// guarantees this via the outcode test.
void AddNearPlaneIntersection( const Vector4& a, const Vector4& b, std::vector<Vector4>& points )
{
	float denominator = a.z - b.z;
	if( fabsf( denominator ) <= CLIP_EPSILON )
	{
		return;
	}

	float t = a.z / denominator;
	Vector4 point = Lerp( a, b, t );
	if( CanPerspectiveDivide( point ) )
	{
		points.push_back( point );
	}
}

bool ProjectClipPoint( const Vector4& point, const TriViewport& viewport, Vector3& projected )
{
	if( !CanPerspectiveDivide( point ) )
	{
		return false;
	}

	float reciprocalW = 1.0f / point.w;
	projected.x = viewport.x + ( 1.0f + point.x * reciprocalW ) * 0.5f * viewport.width;
	projected.y = viewport.y + ( 1.0f - point.y * reciprocalW ) * 0.5f * viewport.height;
	projected.z = viewport.minZ + point.z * reciprocalW * ( viewport.maxZ - viewport.minZ );
	return true;
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

	Vector4 clipCorners[8];
	uint32_t outcodes[8];
	uint32_t combinedOutcode = ~0u;
	for( int i = 0; i < 8; ++i )
	{
		clipCorners[i] = TransformPointToClip( corners[i], viewProjection );
		outcodes[i] = ClipOutcode( clipCorners[i] );
		combinedOutcode &= outcodes[i];
	}

	// A surviving bit means every corner is outside the same plane
	if( combinedOutcode != 0 )
	{
		return false;
	}

	std::vector<Vector4> projectablePoints;
	projectablePoints.reserve( 20 );
	for( int i = 0; i < 8; ++i )
	{
		if( !( outcodes[i] & CLIP_NEAR ) && CanPerspectiveDivide( clipCorners[i] ) )
		{
			projectablePoints.push_back( clipCorners[i] );
		}
	}

	static const int EDGES[12][2] = {
		{ 0, 1 }, { 1, 3 }, { 3, 2 }, { 2, 0 }, { 7, 6 }, { 6, 4 }, { 4, 5 }, { 5, 7 }, { 0, 7 }, { 1, 6 }, { 2, 5 }, { 3, 4 }
	};

	for( int i = 0; i < 12; ++i )
	{
		// XOR: the edge endpoints straddle the near plane
		if( ( outcodes[EDGES[i][0]] ^ outcodes[EDGES[i][1]] ) & CLIP_NEAR )
		{
			AddNearPlaneIntersection( clipCorners[EDGES[i][0]], clipCorners[EDGES[i][1]], projectablePoints );
		}
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

	for( const Vector4& point : projectablePoints )
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

bool ClampToScreenMargin( const TriViewport& viewport, float margin, float& x, float& y, float& width, float& height )
{
	if( margin <= 0.0f )
	{
		return true;
	}

	float left = static_cast<float>( viewport.x ) + margin;
	float top = static_cast<float>( viewport.y ) + margin;
	float right = static_cast<float>( viewport.x ) + static_cast<float>( viewport.width ) - margin;
	float bottom = static_cast<float>( viewport.y ) + static_cast<float>( viewport.height ) - margin;

	float minX = std::max( x, left );
	float minY = std::max( y, top );
	float maxX = std::min( x + width, right );
	float maxY = std::min( y + height, bottom );

	if( maxX <= minX || maxY <= minY )
	{
		return false;
	}

	x = minX;
	y = minY;
	width = maxX - minX;
	height = maxY - minY;
	return true;
}

float ClampProjectedSize( float size, float minSize, float maxSize )
{
	if( minSize > 0.0f && size < minSize )
	{
		return minSize;
	}
	if( maxSize > 0.0f && size > maxSize )
	{
		return maxSize;
	}
	return size;
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
	m_screenMargin( 0.0f ),
	m_cameraDistance( 0 ),
	m_isProjectionValid( false ),
	m_containsCamera( false ),
	m_extendsOffscreen( false ),
	m_coversViewport( false )
{
}


void Tr2ProjectBoundingBoxBracket::UpdateValue( double time )
{
	Vector3 bbMin, bbMax;
	if( !m_object || !m_object->IsBoundingBoxReady() || !m_object->GetWorldBoundingBox( bbMin, bbMax ) )
	{
		SetEmptyProjection();
		return;
	}

	const Vector3 center = ( bbMax + bbMin ) * 0.5f;
	const Vector3 viewPosition = Tr2Renderer::GetViewPosition();
	m_cameraDistance = Length( viewPosition - center );

	const TriViewport& viewport = Tr2Renderer::GetViewport();
	if( BoundingBoxIsInside( bbMin, bbMax, viewPosition ) )
	{
		SetFullViewportProjection( viewport );
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

	ConstrainProjection( center, viewProjection, viewport );
	PublishProjection( viewport );
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
}
void Tr2ProjectBoundingBoxBracket::SetFullViewportProjection( const TriViewport& viewport )
{
	m_projectedX = static_cast<float>( viewport.x );
	m_projectedY = static_cast<float>( viewport.y );
	m_projectedZ = viewport.minZ;
	m_projectedWidth = static_cast<float>( viewport.width );
	m_projectedHeight = static_cast<float>( viewport.height );
	if( !ClampToScreenMargin( viewport, m_screenMargin, m_projectedX, m_projectedY, m_projectedWidth, m_projectedHeight ) )
	{
		SetEmptyProjection();
		return;
	}
	m_isProjectionValid = true;
	m_containsCamera = true;
	m_extendsOffscreen = true;
	m_coversViewport = true;
	UpdateBracket();
}
void Tr2ProjectBoundingBoxBracket::ConstrainProjection( const Vector3& center, const Matrix& viewProjection, const TriViewport& viewport )
{
	float centerX = m_projectedX + m_projectedWidth * 0.5f;
	float centerY = m_projectedY + m_projectedHeight * 0.5f;
	if( m_maxProjectedWidth > 0.0f || m_maxProjectedHeight > 0.0f )
	{
		// Bounded brackets are anchored on the projected 3d box center, not the projected
		// rect center, unless the box center is behind the near plane.
		Vector4 clipCenter = TransformPointToClip( center, viewProjection );
		Vector3 projectedCenter;
		if( clipCenter.z >= 0.0f && clipCenter.w > 0.0f && ProjectClipPoint( clipCenter, viewport, projectedCenter ) )
		{
			centerX = projectedCenter.x;
			centerY = projectedCenter.y;
		}
	}

	m_projectedWidth = ClampProjectedSize( m_projectedWidth, m_minProjectedWidth, m_maxProjectedWidth );
	m_projectedHeight = ClampProjectedSize( m_projectedHeight, m_minProjectedHeight, m_maxProjectedHeight );

	m_projectedX = centerX - m_projectedWidth * 0.5f;
	m_projectedY = centerY - m_projectedHeight * 0.5f;

	if( m_integerCoordinates )
	{
		m_projectedX = floor( m_projectedX + 0.5f );
		m_projectedY = floor( m_projectedY + 0.5f );
		m_projectedWidth = floor( m_projectedWidth + 0.5f );
		m_projectedHeight = floor( m_projectedHeight + 0.5f );
	}
}
void Tr2ProjectBoundingBoxBracket::PublishProjection( const TriViewport& viewport )
{
	if( m_projectedWidth <= 0.0f || m_projectedHeight <= 0.0f )
	{
		SetEmptyProjection();
		return;
	}

	if( !ClampToScreenMargin( viewport, m_screenMargin, m_projectedX, m_projectedY, m_projectedWidth, m_projectedHeight ) )
	{
		SetEmptyProjection();
		return;
	}

	m_isProjectionValid = true;
	UpdateBracket();

	if( g_debugRenderer )
	{
		int x = static_cast<int>( m_projectedX );
		int y = static_cast<int>( m_projectedY );
		g_debugRenderer->Printf( x, y, 0xffffffff, "%S", m_name.c_str() );
		y += 16;
		g_debugRenderer->Printf( x, y, 0xffffffff, "(%5.2f, %5.2f)", m_projectedWidth, m_projectedHeight );
	}
}
