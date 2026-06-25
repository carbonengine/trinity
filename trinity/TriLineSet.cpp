// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriLineSet.h"
#include "Shader/Tr2Effect.h"
#include "Tr2Renderer.h"

#if !defined( D3DTARGET ) || D3DTARGET == 9

// ------------------------------------------------------------------------------------------------------
// Description:
//   Helper function to convert RGBA color to BGRA.
// Attributes:
//   color - color in RGBA format
// Return value:
//   color in ARGB
// ------------------------------------------------------------------------------------------------------
inline unsigned SwizzleColor( unsigned color )
{
	return ( ( color & 0xff0000 ) >> 16 ) | ( color & 0xff00ff00 ) | ( ( color & 0xff ) << 16 );
}

TriLineSet::TriLineSet( IRoot* lockobj ) :
	m_defaultColor( 0xffffffff ),
	m_zEnable( true ),
	m_transform( IdentityMatrix() )
{
}

void TriLineSet::AddDefaultColor( const Vector3& from, const Vector3& to )
{
	size_t newSize = m_vertices.size() + 2;
	if( newSize > 100000 )
	{
		return;
	}

	m_vertices.resize( newSize );

	TriDebugResourceHelper::VertexPosColor& vFrom = m_vertices[newSize - 2];
	vFrom.m_pos = from;
	vFrom.m_color = SwizzleColor( m_defaultColor );

	TriDebugResourceHelper::VertexPosColor& vTo = m_vertices[newSize - 1];
	vTo.m_pos = to;
	vTo.m_color = SwizzleColor( m_defaultColor );
}

void TriLineSet::Add( const Vector3& from, uint32_t colorFrom, const Vector3& to, uint32_t colorTo )
{
	size_t newSize = m_vertices.size() + 2;
	if( newSize > 100000 )
	{
		return;
	}

	m_vertices.resize( newSize );

	TriDebugResourceHelper::VertexPosColor& vFrom = m_vertices[newSize - 2];
	vFrom.m_pos = from;
	vFrom.m_color = SwizzleColor( colorFrom );

	TriDebugResourceHelper::VertexPosColor& vTo = m_vertices[newSize - 1];
	vTo.m_pos = to;
	vTo.m_color = SwizzleColor( colorTo );
}

void TriLineSet::AddTransformed( const Matrix& transform, const Vector3& from, uint32_t colorFrom, const Vector3& to, uint32_t colorTo )
{
	Vector3 newFrom, newTo;
	newFrom = TransformCoord( from, transform );
	newTo = TransformCoord( to, transform );

	Add( newFrom, colorFrom, newTo, colorTo );
}

void TriLineSet::AddSphere( const Vector3& center, float radius, int segments, uint32_t color )
{
	if( segments < 4 )
	{
		segments = 4;
	}

	if( segments % 2 == 1 )
	{
		++segments;
	}

	float step = ( XM_PI * 2.0f ) / (float)segments;
	float halfStep = step * 0.5f;

	float curRadius = 0.0f;
	float curY = 0.0f;
	for( int i = 0; i < segments; ++i )
	{
		float fi = (float)i;

		float nextRadius = sin( fi * halfStep ) * radius;
		float nextY = cos( fi * halfStep ) * radius;

		for( int j = 0; j < segments; ++j )
		{
			float t0 = (float)j * step;
			float x0 = sin( t0 ) * curRadius;
			float z0 = cos( t0 ) * curRadius;

			float t1 = t0 + step;
			float x1 = sin( t1 ) * curRadius;
			float z1 = cos( t1 ) * curRadius;

			Vector3 from;
			from.x = x0 + center.x;
			from.y = center.y - curY;
			from.z = z0 + center.z;

			Vector3 to;
			to.x = x1 + center.x;
			to.y = from.y;
			to.z = z1 + center.z;

			Add( from, color, to, color );

			Vector3 nextTo;
			nextTo.x = center.x + sin( t0 ) * nextRadius;
			nextTo.y = center.y - nextY;
			nextTo.z = center.z + cos( t0 ) * nextRadius;

			Add( from, color, nextTo, color );

			from.y = center.y + curY;
			to.y = from.y;

			Add( from, color, to, color );

			nextTo.y = center.y + nextY;

			Add( from, color, nextTo, color );
		}

		curRadius = nextRadius;
		curY = nextY;
	}
}

void TriLineSet::AddCylinder( const Vector3& start, const Vector3& end, float radius, int segments, uint32_t color )
{
	Vector3 z = start - end;
	float length = Length( z );
	z /= length;

	Vector3 up( 0.0f, 1.0f, 0.0f );
	if( fabs( z.y ) > 0.99f )
	{
		up = Vector3( 1.0f, 0.0f, 0.0f );
	}

	Vector3 x = Normalize( Cross( up, z ) );

	Vector3 y = Cross( z, x );

	Matrix m;
	m._11 = x.x;
	m._12 = x.y;
	m._13 = x.z;
	m._14 = 0.0f;

	m._21 = y.x;
	m._22 = y.y;
	m._23 = y.z;
	m._24 = 0.0f;

	m._31 = z.x;
	m._32 = z.y;
	m._33 = z.z;
	m._34 = 0.0f;

	m._41 = end.x;
	m._42 = end.y;
	m._43 = end.z;
	m._44 = 1.0f;

	Matrix scale = ScalingMatrix( radius, radius, length );
	m = scale * m;

	if( segments < 4 )
	{
		segments = 4;
	}

	if( segments % 2 == 1 )
	{
		++segments;
	}

	float step = ( XM_PI * 2.0f ) / (float)segments;
	for( int i = 0; i < segments; ++i )
	{
		float x = cosf( i * step );
		float y = sinf( i * step );

		Vector3 from( x, y, 0.0f );
		Vector3 to( x, y, 1.0f );

		from = TransformCoord( from, m );
		to = TransformCoord( to, m );

		Add( from, color, to, color );
		Add( from, color, end, color );
		Add( to, color, start, color );

		float x2 = cosf( i * step + step );
		float y2 = sinf( i * step + step );

		Vector3 from2( x2, y2, 0.0f );
		Vector3 to2( x2, y2, 1.0f );

		from2 = TransformCoord( from2, m );
		to2 = TransformCoord( to2, m );

		Add( from, color, from2, color );
		Add( to, color, to2, color );
	}
}

void TriLineSet::AddCone( const Vector3& start, const Vector3& end, float radius, int segments, uint32_t color )
{
	Vector3 z = start - end;
	float length = Length( z );
	z /= length;

	Vector3 up( 0.0f, 1.0f, 0.0f );
	if( fabs( z.y ) > 0.99f )
	{
		up = Vector3( 1.0f, 0.0f, 0.0f );
	}

	Vector3 x = Normalize( Cross( up, z ) );

	Vector3 y = Cross( z, x );

	Matrix m;
	m._11 = x.x;
	m._12 = x.y;
	m._13 = x.z;
	m._14 = 0.0f;

	m._21 = y.x;
	m._22 = y.y;
	m._23 = y.z;
	m._24 = 0.0f;

	m._31 = z.x;
	m._32 = z.y;
	m._33 = z.z;
	m._34 = 0.0f;

	m._41 = end.x;
	m._42 = end.y;
	m._43 = end.z;
	m._44 = 1.0f;

	Matrix scale = ScalingMatrix( radius, radius, length );
	m = scale * m;

	if( segments < 4 )
	{
		segments = 4;
	}

	if( segments % 2 == 1 )
	{
		++segments;
	}

	float step = ( XM_PI * 2.0f ) / (float)segments;
	for( int i = 0; i < segments; ++i )
	{
		float x = cosf( i * step );
		float y = sinf( i * step );

		Vector3 from( x, y, 0.0f );
		Vector3 to( 0.0f, 0.0f, 1.0f );

		from = TransformCoord( from, m );
		to = TransformCoord( to, m );

		Add( from, color, to, color );
		Add( from, color, end, color );
		Add( to, color, start, color );

		float x2 = cosf( i * step + step );
		float y2 = sinf( i * step + step );

		Vector3 from2( x2, y2, 0.0f );
		Vector3 to2( 0.0f, 0.0f, 1.0f );

		from2 = TransformCoord( from2, m );
		to2 = TransformCoord( to2, m );

		Add( from, color, from2, color );
		Add( to, color, to2, color );
	}
}

void TriLineSet::AddBox( const Vector3& min, const Vector3& max, uint32_t color )
{
	Vector3 minA( max.x, min.y, min.z );
	Vector3 minB( min.x, max.y, min.z );
	Vector3 minC( max.x, max.y, min.z );
	Vector3 maxA( max.x, min.y, max.z );
	Vector3 maxB( min.x, max.y, max.z );
	Vector3 maxC( min.x, min.y, max.z );

	Add( min, color, minA, color );
	Add( min, color, minB, color );
	Add( minC, color, minB, color );
	Add( minA, color, minC, color );

	Add( max, color, maxA, color );
	Add( max, color, maxB, color );
	Add( maxC, color, maxB, color );
	Add( maxA, color, maxC, color );

	Add( min, color, maxC, color );
	Add( max, color, minC, color );
	Add( minB, color, maxB, color );
	Add( minA, color, maxA, color );
}

void TriLineSet::AddOrientedBox( const Matrix& transform, const Vector3& min, const Vector3& max, uint32_t color )
{
	Vector3 minA( max.x, min.y, min.z );
	Vector3 minB( min.x, max.y, min.z );
	Vector3 minC( max.x, max.y, min.z );
	Vector3 maxA( max.x, min.y, max.z );
	Vector3 maxB( min.x, max.y, max.z );
	Vector3 maxC( min.x, min.y, max.z );

	AddTransformed( transform, min, color, minA, color );
	AddTransformed( transform, min, color, minB, color );
	AddTransformed( transform, minC, color, minB, color );
	AddTransformed( transform, minA, color, minC, color );

	AddTransformed( transform, max, color, maxA, color );
	AddTransformed( transform, max, color, maxB, color );
	AddTransformed( transform, maxC, color, maxB, color );
	AddTransformed( transform, maxA, color, maxC, color );

	AddTransformed( transform, min, color, maxC, color );
	AddTransformed( transform, max, color, minC, color );
	AddTransformed( transform, minB, color, maxB, color );
	AddTransformed( transform, minA, color, maxA, color );
}

void TriLineSet::AddOrientedBox( const Matrix& boxMatrix, uint32_t color )
{
	Vector3 min( -1.f, -1.f, -1.f );
	Vector3 minA( 1.f, -1.f, -1.f );
	Vector3 minB( -1.f, 1.f, -1.f );
	Vector3 minC( 1.f, 1.f, -1.f );
	Vector3 maxA( 1.f, -1.f, 1.f );
	Vector3 maxB( -1.f, 1.f, 1.f );
	Vector3 maxC( -1.f, -1.f, 1.f );
	Vector3 max( 1.f, 1.f, 1.f );

	min = TransformCoord( min, boxMatrix );
	minA = TransformCoord( minA, boxMatrix );
	minB = TransformCoord( minB, boxMatrix );
	minC = TransformCoord( minC, boxMatrix );
	max = TransformCoord( max, boxMatrix );
	maxA = TransformCoord( maxA, boxMatrix );
	maxB = TransformCoord( maxB, boxMatrix );
	maxC = TransformCoord( maxC, boxMatrix );

	Add( min, color, minA, color );
	Add( min, color, minB, color );
	Add( minC, color, minB, color );
	Add( minA, color, minC, color );

	Add( max, color, maxA, color );
	Add( max, color, maxB, color );
	Add( maxC, color, maxB, color );
	Add( maxA, color, maxC, color );

	Add( min, color, maxC, color );
	Add( max, color, minC, color );
	Add( minB, color, maxB, color );
	Add( minA, color, maxA, color );
}

void TriLineSet::Clear()
{
	m_vertices.clear();
}

void TriLineSet::SetCurrentColor( uint32_t val )
{
	size_t numPoints = m_vertices.size();
	val = SwizzleColor( val );
	for( size_t i = 0; i < numPoints; i++ )
	{
		m_vertices[i].m_color = val;
	}
}

void TriLineSet::Render( Tr2RenderContext& renderContext )
{
	if( m_vertices.empty() )
	{
		return;
	}

	renderContext.m_esm.ApplyVertexDeclaration( g_debugResourceHelper.GetVertexPosColorDecl() );

	Tr2Renderer::SetWorldTransform( m_transform );

	g_debugResourceHelper.GetLineEffect()->Render( this, renderContext );
}

void TriLineSet::SubmitGeometry( Tr2RenderContext& renderContext )
{
	using namespace Tr2RenderContextEnum;

	uint32_t numLines = (uint32_t)m_vertices.size() / 2;
	void* verts = &m_vertices[0];
	uint32_t stride = sizeof( TriDebugResourceHelper::VertexPosColor );

	renderContext.SetTopology( TOP_LINES );
	renderContext.DrawPrimitiveUP( numLines, verts, stride );
}

void TriLineSet::AddLines( std::vector<std::pair<Vector3, Vector3>> lines )
{
	for( auto it = lines.begin(); it != lines.end(); ++it )
	{
		AddDefaultColor( it->first, it->second );
	}
}

void TriLineSet::RenderFromScript()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	Render( renderContext );
}

#endif
