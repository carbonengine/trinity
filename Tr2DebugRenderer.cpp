////////////////////////////////////////////////////////////
//
//    Created:   December 2016
//    Copyright: CCP 2016
//

#include "StdAfx.h"
#include "Tr2DebugRenderer.h"
#include "Shader/Tr2Effect.h"
#include "Shader/Tr2Shader.h"
#include "Tr2Renderer.h"
#include "TriDebugTextRenderer.h"


namespace
{

Matrix LineTransform( const Vector3& start, const Vector3& end )
{
	Matrix transform = IdentityMatrix();
	transform.GetY() = XMVector3Normalize( ( end - start ) * 0.5 );
	if( std::abs( transform.GetY().x ) > 0.9f )
	{
		transform.GetX() = Vector3( 0, 0, 1 );
	}
	else
	{
		transform.GetX() = Vector3( 1, 0, 0 );
	}
	transform.GetZ() = XMVector3Normalize( XMVector3Cross( transform.GetX(), transform.GetY() ) );
	transform.GetX() = XMVector3Cross( transform.GetY(), transform.GetZ() );
	transform.GetTranslation() = ( start + end ) * 0.5;
	return transform;
}

Vector2 LineNormal( const Vector2& p0, const Vector2& p1 )
{
	Vector2 n0( p1 - p0 );
	return Vector2( XMVector2Normalize( Vector2( -n0.y, n0.x ) ) );
}

uint32_t GetVertexDeclarationHandle( Tr2RenderContext& renderContext )
{
	static Tr2VertexDefinition definition;
	if( definition.empty() )
	{
		definition.Add( definition.FLOAT32_3, definition.POSITION );
		definition.Add( definition.FLOAT32_3, definition.NORMAL );
		definition.Add( definition.FLOAT32_2, definition.TEXCOORD, 0 );
		definition.Add( definition.UBYTE_4_NORM, definition.COLOR, 1 );
		definition.Add( definition.UBYTE_4_NORM, definition.COLOR, 2 );
	}

	return renderContext.m_esm.GetVertexDeclarationHandle( definition );
}

TriDebugTextRenderer& GetDebugTextRenderer()
{
	static TriDebugTextRenderer renderer;
	return renderer;
}

uint32_t GetAutoSelectedColor( uint32_t color )
{
	auto c = Color( color );
	c.r = std::min( 1.f, c.r + 0.7f );
	c.g = std::min( 1.f, c.g + 0.7f );
	c.b = std::min( 1.f, c.b + 0.7f );
	return c;
}

}

Tr2DebugObjectReference::Tr2DebugObjectReference( IRoot* object )
	:m_object( object ),
	m_area( 0 )
{
}

Tr2DebugObjectReference::Tr2DebugObjectReference( IRoot* object, uint32_t area )
	:m_object( object ),
	m_area( area )
{
}

Tr2DebugObjectReference::operator bool() const
{
	return m_object != nullptr;
}

bool Tr2DebugObjectReference::operator<( const Tr2DebugObjectReference& other ) const
{
	if( m_object < other.m_object )
	{
		return true;
	}
	if( m_object > other.m_object )
	{
		return false;
	}
	return m_area < other.m_area;
}

bool Tr2DebugObjectReference::operator==( const Tr2DebugObjectReference& other ) const
{
	return m_object == other.m_object && m_area == other.m_area;
}

bool Tr2DebugObjectReference::operator!=( const Tr2DebugObjectReference& other ) const
{
	return m_object != other.m_object || m_area != other.m_area;
}



Tr2DebugRenderer::Vertex::Vertex()
{
}

Tr2DebugRenderer::Vertex::Vertex( const Vector3& position, Tr2DebugColor color, bool selected, size_t objectIndex )
	:m_position( position ),
	m_normal( 0, 0, 0 ),
	m_color( selected ? color.m_colorSelected : color.m_color ),
	m_zFailColor( selected ? color.m_zFailColorSelected : color.m_zFailColor ),
	m_line( 1.f )
{
	m_object = float( objectIndex + 1 );
}

Tr2DebugRenderer::Vertex::Vertex( const Vector3& position, const Vector3& normal, Tr2DebugColor color, bool selected, size_t objectIndex )
	:m_position( position ),
	m_normal( XMVector3Normalize( normal ) ),
	m_color( selected ? color.m_colorSelected : color.m_color ),
	m_zFailColor( selected ? color.m_zFailColorSelected : color.m_zFailColor ),
	m_line( 0 )
{
	m_object = float( objectIndex + 1 );
}


Tr2DebugRenderer::Tr2DebugRenderer( IRoot* lockobj )
	:m_pickBuffer( NULL,  Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT, 1 )
{
	m_effect.CreateInstance();
	m_effect->SetEffectPathName( "res:/Graphics/Effect/Managed/Utility/DebugPrimitive.fx" );
	m_pickingEffect.CreateInstance();
	m_pickingEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Utility/DebugPrimitivePicking.fx" );

	m_pickBuffer.PrepareResources();
	m_pickBuffer.SetClearColor( 0 );
}

bool Tr2DebugRenderer::HasOption( IRoot* owner, const char* option ) const
{
	auto found = m_options.find( owner );
	if( found == m_options.end() )
	{
		return m_defaultOptions.find( option ) != m_defaultOptions.end();
	}
	return found->second.find( option ) != found->second.end();
}

bool Tr2DebugRenderer::IsSelected( IRoot* owner ) const
{
	return m_selectedObjects.find( owner ) != m_selectedObjects.end();
}

bool Tr2DebugRenderer::IsSelected( Tr2DebugObjectReference owner ) const
{
	return m_selectedObjects.find( owner ) != m_selectedObjects.end();
}

void Tr2DebugRenderer::DrawLine( Tr2DebugObjectReference owner, const Vector3& from, const Vector3& to, Tr2DebugColor color )
{
	if( m_objectLineOffsets.empty() || m_objectLineOffsets.back().first != owner )
	{
		m_objectLineOffsets.push_back( std::make_pair( owner, m_lines.size() ) );
	}
	m_lines.push_back( Vertex( from, color, IsSelected( owner ), m_objectLineOffsets.size() - 1 ) );
	m_lines.push_back( Vertex( to, color, IsSelected( owner ), m_objectLineOffsets.size() - 1 ) );
}

void Tr2DebugRenderer::DrawTriangle( 
	Tr2DebugObjectReference owner, 
	const Vector3& vertex1, 
	const Vector3& normal1, 
	const Vector3& vertex2, 
	const Vector3& normal2, 
	const Vector3& vertex3, 
	const Vector3& normal3, 
	Tr2DebugColor color )
{
	if( m_objectTriangleOffsets.empty() || m_objectTriangleOffsets.back().first != owner )
	{
		m_objectTriangleOffsets.push_back( std::make_pair( owner, m_triangles.size() ) );
	}
	m_triangles.push_back( Vertex( vertex1, normal1, color, IsSelected( owner ), m_objectTriangleOffsets.size() - 1 ) );
	m_triangles.push_back( Vertex( vertex2, normal2, color, IsSelected( owner ), m_objectTriangleOffsets.size() - 1 ) );
	m_triangles.push_back( Vertex( vertex3, normal3, color, IsSelected( owner ), m_objectTriangleOffsets.size() - 1 ) );
}

void Tr2DebugRenderer::DrawTriangle( 
	Tr2DebugObjectReference owner, 
	const Vector3& vertex1, 
	const Vector3& vertex2, 
	const Vector3& vertex3, 
	const Vector3& normal, 
	Tr2DebugColor color )
{
	DrawTriangle( owner, vertex1, normal, vertex2, normal, vertex3, normal, color );
}


void Tr2DebugRenderer::DrawBox( Tr2DebugObjectReference owner, const Vector3& min, const Vector3& max, Effect effect, Tr2DebugColor color )
{
	DrawBox( owner, IdentityMatrix(), min, max, effect, color );
}

void Tr2DebugRenderer::DrawBox( Tr2DebugObjectReference owner, const Matrix& transform, const Vector3& min, const Vector3& max, Effect effect, Tr2DebugColor color )
{
	Vector3 v000( XMVector3TransformCoord( min, transform ) );
	Vector3 v100( XMVector3TransformCoord( Vector3( max.x, min.y, min.z ), transform ) );
	Vector3 v010( XMVector3TransformCoord( Vector3( min.x, max.y, min.z ), transform ) );
	Vector3 v110( XMVector3TransformCoord( Vector3( max.x, max.y, min.z ), transform ) );
	Vector3 v001( XMVector3TransformCoord( Vector3( min.x, min.y, max.z ), transform ) );
	Vector3 v101( XMVector3TransformCoord( Vector3( max.x, min.y, max.z ), transform ) );
	Vector3 v011( XMVector3TransformCoord( Vector3( min.x, max.y, max.z ), transform ) );
	Vector3 v111( XMVector3TransformCoord( max, transform ) );

	if( effect == Wireframe )
	{
		DrawLine( owner, v000, v100, color );
		DrawLine( owner, v000, v010, color );
		DrawLine( owner, v110, v010, color );
		DrawLine( owner, v110, v100, color );

		DrawLine( owner, v001, v101, color );
		DrawLine( owner, v001, v011, color );
		DrawLine( owner, v111, v011, color );
		DrawLine( owner, v111, v101, color );
	
		DrawLine( owner, v000, v001, color );
		DrawLine( owner, v100, v101, color );
		DrawLine( owner, v010, v011, color );
		DrawLine( owner, v110, v111, color );
	}
	else
	{
		Vector3 nx, ny, nz;
		if( effect == Lit )
		{
			nx = Vector3( XMVector3TransformNormal( Vector3( 1, 0, 0 ), transform ) );
			ny = Vector3( XMVector3TransformNormal( Vector3( 0, 1, 0 ), transform ) );
			nz = Vector3( XMVector3TransformNormal( Vector3( 0, 0, 1 ), transform ) );
		}
		else
		{
			nx = Vector3( 0, 0, 0 );
			ny = Vector3( 0, 0, 0 );
			nz = Vector3( 0, 0, 0 );
		}

		DrawTriangle( owner, v000, v010, v100, -nz, color );
		DrawTriangle( owner, v100, v010, v110, -nz, color );

		DrawTriangle( owner, v001, v101, v011, nz, color );
		DrawTriangle( owner, v101, v111, v011, nz, color );

		DrawTriangle( owner, v000, v001, v010, -nx, color );
		DrawTriangle( owner, v001, v011, v010, -nx, color );

		DrawTriangle( owner, v100, v110, v101, nx, color );
		DrawTriangle( owner, v101, v110, v111, nx, color );


		DrawTriangle( owner, v000, v100, v001, -ny, color );
		DrawTriangle( owner, v001, v100, v101, -ny, color );

		DrawTriangle( owner, v010, v011, v110, ny, color );
		DrawTriangle( owner, v011, v111, v110, ny, color );
	}
}

void Tr2DebugRenderer::DrawSphere( Tr2DebugObjectReference owner, const Vector4& sphere, uint32_t segments, Effect effect, Tr2DebugColor color )
{
	DrawSphere( owner, IdentityMatrix(), Vector3( sphere.x, sphere.y, sphere.z ), sphere.w, segments, effect, color );
}

void Tr2DebugRenderer::DrawSphere( Tr2DebugObjectReference owner, const Vector3& center, float radius, uint32_t segments, Effect effect, Tr2DebugColor color )
{
	DrawSphere( owner, IdentityMatrix(), center, radius, segments, effect, color );
}

void Tr2DebugRenderer::DrawSphere( Tr2DebugObjectReference owner, const Matrix& transform, uint32_t segments, Effect effect, Tr2DebugColor color )
{
	DrawSphere( owner, transform, Vector3( 0., 0.f, 0.f ), 1.0f, segments, effect, color );
}

void Tr2DebugRenderer::DrawSphere( Tr2DebugObjectReference owner, const Matrix& transform, float radius, uint32_t segments, Effect effect, Tr2DebugColor color )
{
	DrawSphere( owner, transform, Vector3( 0., 0.f, 0.f ), radius, segments, effect, color );
}

void Tr2DebugRenderer::DrawSphere( Tr2DebugObjectReference owner, const Matrix& transform, const Vector3& center, float radius, uint32_t segments, Effect effect, Tr2DebugColor color )
{
	if( segments < 4 )
	{
		segments = 4;
	}

	auto Coords = [&]( float alpha, float betta )->Vector3
	{
		Vector3 v;
		v.y = sin( alpha - XM_PI / 2 );
		float ca = cos( alpha - XM_PI / 2 );
		v.x = sin( betta ) * ca;
		v.z = cos( betta ) * ca;
		return v;
	};


	for( uint32_t j = 0; j < segments / 2; ++j )
	{
		float alpha0 = float( j ) / float( segments / 2 ) * XM_PI;
		float alpha1 = float( j + 1 ) / float( segments / 2 ) * XM_PI;
		for( uint32_t i = 0; i < segments; ++i )
		{
			float betta0 = float( i ) / float( segments ) * 2 * XM_PI;
			float betta1 = float( i + 1 ) / float( segments ) * 2 * XM_PI;

			Vector3 p00 = Coords( alpha0, betta0 );
			Vector3 p10 = Coords( alpha0, betta1 );
			Vector3 p01 = Coords( alpha1, betta0 );
			Vector3 p11 = Coords( alpha1, betta1 );

			switch( effect )
			{
			case Wireframe:
				DrawLine( 
					owner, 
					Vector3( XMVector3TransformCoord( p00 * radius + center, transform ) ), 
					Vector3( XMVector3TransformCoord( p01 * radius + center, transform ) ), 
					color );
				DrawLine( 
					owner, 
					Vector3( XMVector3TransformCoord( p00 * radius + center, transform ) ), 
					Vector3( XMVector3TransformCoord( p10 * radius + center, transform ) ), 
					color );
				break;
			case Solid:
				DrawTriangle( 
					owner, 
					Vector3( XMVector3TransformCoord( p00 * radius + center, transform ) ), 
					Vector3( XMVector3TransformCoord( p10 * radius + center, transform ) ), 
					Vector3( XMVector3TransformCoord( p01 * radius + center, transform ) ), 
					Vector3( 0, 0, 0 ), 
					color );
				DrawTriangle( 
					owner, 
					Vector3( XMVector3TransformCoord( p10 * radius + center, transform ) ), 
					Vector3( XMVector3TransformCoord( p11 * radius + center, transform ) ), 
					Vector3( XMVector3TransformCoord( p01 * radius + center, transform ) ), 
					Vector3( 0, 0, 0 ), 
					color );
				break;
			default:
				DrawTriangle( 
					owner, 
					Vector3( XMVector3TransformCoord( p00 * radius + center, transform ) ), 
					Vector3( XMVector3TransformNormal( p00, transform ) ), 
					Vector3( XMVector3TransformCoord( p10 * radius + center, transform ) ), 
					Vector3( XMVector3TransformNormal( p10, transform ) ), 
					Vector3( XMVector3TransformCoord( p01 * radius + center, transform ) ), 
					Vector3( XMVector3TransformNormal( p01, transform ) ), 
					color );
				DrawTriangle( 
					owner, 
					Vector3( XMVector3TransformCoord( p10 * radius + center, transform ) ), 
					Vector3( XMVector3TransformNormal( p10, transform ) ), 
					Vector3( XMVector3TransformCoord( p11 * radius + center, transform ) ), 
					Vector3( XMVector3TransformNormal( p11, transform ) ), 
					Vector3( XMVector3TransformCoord( p01 * radius + center, transform ) ), 
					Vector3( XMVector3TransformNormal( p01, transform ) ), 
					color );
			}
		}
	}
}

void Tr2DebugRenderer::DrawCylinder( Tr2DebugObjectReference owner, const Matrix& transform, float radius, float height, uint32_t segments, Effect effect, Tr2DebugColor color )
{
	const Vector2 vertices[] = { Vector2( -height / 2, 0 ), Vector2( -height / 2, radius ), Vector2( height / 2, radius ), Vector2( height / 2, 0 ) };
	const Vector2 normals[] = { Vector2( -1, 0 ), Vector2( -1, 0 ), Vector2( 0, 1 ), Vector2( 0, 1 ), Vector2( 1, 0 ), Vector2( 1, 0 ) };
	DrawExtrusionShape( owner, transform, vertices, normals, sizeof( vertices ) / sizeof( vertices[0] ), segments, effect, color );
}

void Tr2DebugRenderer::DrawCylinder( Tr2DebugObjectReference owner, const Vector3& cap0, const Vector3& cap1, float radius, uint32_t segments, Effect effect, Tr2DebugColor color )
{
	DrawCylinder( owner, IdentityMatrix(), cap0, cap1, radius, segments, effect, color );
}

void Tr2DebugRenderer::DrawCylinder( Tr2DebugObjectReference owner, const Matrix& transform, const Vector3& cap0, const Vector3& cap1, float radius, uint32_t segments, Effect effect, Tr2DebugColor color )
{
	Matrix t = LineTransform( cap0, cap1 ) * transform;

	DrawCylinder( owner, t, radius, XMVectorGetX( XMVector3Length( cap1 - cap0 ) ), segments, effect, color );
}

void Tr2DebugRenderer::DrawCone( Tr2DebugObjectReference owner, const Matrix& transform, float radius, float height, uint32_t segments, Effect effect, Tr2DebugColor color )
{
	const Vector2 vertices[] = { Vector2( -height / 2, 0 ), Vector2( -height / 2, radius ), Vector2( height / 2, 0 ) };
	Vector2 n( vertices[2] - vertices[1] );
	n = XMVector2Normalize( Vector2( -n.y, n.x ) );
	const Vector2 normals[] = { Vector2( -1, 0 ), Vector2( -1, 0 ), n, n };
	DrawExtrusionShape( owner, transform, vertices, normals, sizeof( vertices ) / sizeof( vertices[0] ), segments, effect, color );
}

void Tr2DebugRenderer::DrawCone( Tr2DebugObjectReference owner, const Vector3& base, const Vector3& focal, float radius, uint32_t segments, Effect effect, Tr2DebugColor color )
{
	Matrix transform = LineTransform( base, focal );

	DrawCone( owner, transform, radius, XMVectorGetX( XMVector3Length( focal - base ) ), segments, effect, color );
}

void Tr2DebugRenderer::DrawCone( Tr2DebugObjectReference owner, const Matrix& transform, float height, float angle, uint32_t segments, uint32_t coneSegments, Effect effect, Tr2DebugColor color )
{
	coneSegments = std::max( coneSegments, (uint32_t)3 );
	
	std::vector<Vector2> vertices;
	vertices.reserve( coneSegments + 1 );

	std::vector<Vector2> normals;
	normals.reserve( ( coneSegments * 2 - 1 ) * 2 );
	auto tip = Vector2( 0, 0 );
	vertices.push_back( tip );
	normals.push_back( Vector2( -1, 0 ) );

	for( uint32_t i = 0; i < coneSegments; ++i)
	{
		float alpha = (coneSegments - 1 - float( i )) / float( coneSegments - 1 ) * angle;
		vertices.push_back( Vector2( -cos( alpha ) * height, sin( alpha ) * height ) + tip );
		normals.push_back( Vector2( -cos( alpha ), sin( alpha ) ) );	
		if( i != coneSegments-1 )
		{
			normals.push_back( Vector2( -cos( alpha ), sin( alpha ) ) );
		}
	}
	
	DrawExtrusionShape( owner, RotationXMatrix( -XM_PIDIV2 ) * transform, &vertices[0], &normals[0], uint32_t( vertices.size() ), segments, effect, color );
}

void Tr2DebugRenderer::DrawCapsule( Tr2DebugObjectReference owner, const Matrix& transform, float radius, float height, uint32_t segments, Effect effect, Tr2DebugColor color )
{
	if( segments < 3 )
	{
		segments = 3;
	}

	std::vector<Vector2> vertices;
	vertices.reserve( segments * 2 );
	std::vector<Vector2> normals;
	normals.reserve( ( segments * 2 - 1 ) * 2 );

	for( uint32_t i = 0; i < segments; ++i )
	{
		float alpha = float( i ) / float( segments - 1 ) * XM_PI / 2;
		vertices.push_back( Vector2( -cos( alpha ) * radius - height / 2, sin( alpha ) * radius ) );
		normals.push_back( Vector2( -cos( alpha ), sin( alpha ) ) );
		if( i )
		{
			normals.push_back( Vector2( -cos( alpha ), sin( alpha ) ) );
		}
	}
	for( uint32_t i = 0; i < segments; ++i )
	{
		float alpha = XM_PI / 2 - float( i ) / float( segments - 1 ) * XM_PI / 2;
		vertices.push_back( Vector2( cos( alpha ) * radius + height / 2, sin( alpha ) * radius ) );
		normals.push_back( Vector2( cos( alpha ), sin( alpha ) ) );
		if( i + 1 < segments )
		{
			normals.push_back( Vector2( cos( alpha ), sin( alpha ) ) );
		}
	}

	DrawExtrusionShape( owner, transform, &vertices[0], &normals[0], uint32_t( vertices.size() ), segments, effect, color );
}

void Tr2DebugRenderer::DrawCapsule( Tr2DebugObjectReference owner, const Vector3& cap0, const Vector3& cap1, float radius, uint32_t segments, Effect effect, Tr2DebugColor color )
{
	Matrix transform = LineTransform( cap0, cap1 );
	DrawCapsule( owner, transform, radius, XMVectorGetX( XMVector3Length( cap1 - cap0 ) ), segments, effect, color );
}

void Tr2DebugRenderer::DrawArrow( Tr2DebugObjectReference owner, const Vector3& start, const Vector3& end, float radius, float pointerLength, uint32_t segments, Effect effect, Tr2DebugColor color )
{
	Matrix transform = LineTransform( start, end );

	float length = XMVectorGetX( XMVector3Length( start - end ) );

	if( radius <= 0 )
	{
		radius = length * pointerLength * 0.15f;

		const Vector2 vertices[] = { 
			Vector2( length / 2 - pointerLength * length, 0 ), 
			Vector2( length / 2 - pointerLength * length, radius * 2 ), 
			Vector2( length / 2, 0 ) };
		Vector2 n = LineNormal( vertices[1], vertices[2] );
		const Vector2 normals[] = { 
			Vector2( -1, 0 ), 
			Vector2( -1, 0 ), n, 
			n };
		DrawExtrusionShape( owner, transform, vertices, normals, sizeof( vertices ) / sizeof( vertices[0] ), segments, effect, color );
		DrawLine( owner, start, start + transform.GetY() * length * ( 1 - pointerLength ), color );
	}
	else
	{
		const Vector2 vertices[] = { 
			Vector2( -length / 2, 0 ), 
			Vector2( -length / 2, radius ), 
			Vector2( length / 2 - pointerLength * length, radius ), 
			Vector2( length / 2 - pointerLength * length, radius * 2 ), 
			Vector2( length / 2, 0 ) };
		Vector2 n = LineNormal( vertices[2], vertices[3] );
		const Vector2 normals[] = { 
			Vector2( -1, 0 ), 
			Vector2( -1, 0 ), Vector2( 0, 1 ), 
			Vector2( 0, 1 ), Vector2( 0, 1 ), 
			Vector2( 0, 1 ), Vector2( -1, 0 ), 
			Vector2( -1, 0 ), n,
			n };
		DrawExtrusionShape( owner, transform, vertices, normals, sizeof( vertices ) / sizeof( vertices[0] ), segments, effect, color );
	}
}

void Tr2DebugRenderer::DrawDoubleArrow( Tr2DebugObjectReference owner, const Vector3& start, const Vector3& end, float radius, float pointerLength, uint32_t segments, Effect effect, Tr2DebugColor color )
{
	Matrix transform = LineTransform( start, end );

	float length = XMVectorGetX( XMVector3Length( start - end ) );

	if( radius <= 0 )
	{
		radius = length * pointerLength * 0.15f;

		const Vector2 vertices0[] = { 
			Vector2( -length / 2, 0 ), 
			Vector2( -length / 2 + length * pointerLength, radius * 2 ), 
			Vector2( -length / 2 + length * pointerLength, 0 ) };
		Vector2 n0 = LineNormal( vertices0[0], vertices0[1] );
		const Vector2 normals0[] = { 
			n0,
			n0, Vector2( 1, 0 ), 
			Vector2( 1, 0 ) };
		DrawExtrusionShape( owner, transform, vertices0, normals0, sizeof( vertices0 ) / sizeof( vertices0[0] ), segments, effect, color );

		const Vector2 vertices1[] = { 
			Vector2( length / 2 - length * pointerLength, 0 ), 
			Vector2( length / 2 - length * pointerLength, radius * 2 ), 
			Vector2( length / 2, 0 ) };
		Vector2 n1 = LineNormal( vertices1[1], vertices1[2] );
		const Vector2 normals1[] = { 
			Vector2( -1, 0 ),
			Vector2( -1, 0 ), n1,
			n1 };
		DrawExtrusionShape( owner, transform, vertices1, normals1, sizeof( vertices1 ) / sizeof( vertices1[0] ), segments, effect, color );

		DrawLine( owner, start + transform.GetY() * length * pointerLength, start + transform.GetY() * length * ( 1 - pointerLength ), color );
	}
	else
	{
		const Vector2 vertices[] = { 
			Vector2( -length / 2, 0 ), 
			Vector2( -length / 2 + length * pointerLength, radius * 2 ), 
			Vector2( -length / 2 + length * pointerLength, radius ), 
			Vector2( length / 2 - pointerLength * length, radius ), 
			Vector2( length / 2 - pointerLength * length, radius * 2 ), 
			Vector2( length / 2, 0 ) };
		Vector2 n0 = LineNormal( vertices[0], vertices[1] );
		Vector2 n1 = LineNormal( vertices[4], vertices[5] );
		const Vector2 normals[] = { 
			n0,
			n0, Vector2( 1, 0 ), 
			Vector2( 1, 0 ), Vector2( 0, 1 ), 
			Vector2( 0, 1 ), Vector2( -1, 0 ), 
			Vector2( -1, 0 ), n1, 
			n1 };
		DrawExtrusionShape( owner, transform, vertices, normals, sizeof( vertices ) / sizeof( vertices[0] ), segments, effect, color );
	}
}

void Tr2DebugRenderer::DrawSphereArrow( Tr2DebugObjectReference owner, const Vector3& center, const Vector3& direction, float radius, uint32_t segments, Effect effect, Tr2DebugColor color )
{
	if( segments < 3 )
	{
		segments = 3;
	}

	Matrix transform = LineTransform( center - direction, center + direction );

	std::vector<Vector2> vertices;
	vertices.reserve( segments + 4 );
	std::vector<Vector2> normals;
	normals.reserve( ( segments + 4 - 1 ) * 2 );

	float arrowRadius = radius / 5;

	float threshold = std::asin( arrowRadius / radius );
	for( uint32_t i = 0; i < segments; ++i )
	{
		float angle = XM_PI - float( i ) / float( segments - 1 ) * XM_PI;
		bool done = false;
		if( angle < threshold )
		{
			angle = threshold;
			done = true;
		}

		vertices.push_back( Vector2( cos( angle ) * radius, sin( angle ) * radius ) );
		normals.push_back( Vector2( cos( angle ), sin( angle ) ) );
		if( i && !done )
		{
			normals.push_back( Vector2( cos( angle ), sin( angle ) ) );
		}
		if( done )
		{
			break;
		}
	}
	normals.push_back( Vector2( 0, 1 ) );
	vertices.push_back( Vector2( 2 * radius, arrowRadius ) );
	normals.push_back( Vector2( 0, 1 ) );
	
	normals.push_back( Vector2( -1, 0 ) );
	vertices.push_back( Vector2( 2 * radius, 2 * arrowRadius ) );
	normals.push_back( Vector2( -1, 0 ) );

	vertices.push_back( Vector2( 2 * radius + 3 * arrowRadius, 0 ) );
	Vector2 n = LineNormal( vertices[vertices.size() - 2], vertices[vertices.size() - 1] );
	normals.push_back( n );
	normals.push_back( n );

	DrawExtrusionShape( owner, transform, &vertices[0], &normals[0], uint32_t( vertices.size() ), segments, effect, color );
}

void Tr2DebugRenderer::DrawAxis( Tr2DebugObjectReference owner, const Matrix& transform, Effect effect )
{
	DrawArrow( owner, transform.GetTranslation(), transform.GetTranslation() + transform.GetX(), 0, 0.2f, 10, effect, Color( 1, 0, 0, 1 ) );
	DrawArrow( owner, transform.GetTranslation(), transform.GetTranslation() + transform.GetY(), 0, 0.2f, 10, effect, Color( 0, 1, 0, 1 ) );
	DrawArrow( owner, transform.GetTranslation(), transform.GetTranslation() + transform.GetZ(), 0, 0.2f, 10, effect, Color( 0, 0, 1, 1 ) );
}

void Tr2DebugRenderer::DrawExtrusionShape( Tr2DebugObjectReference owner, const Matrix& transform, const Vector2* vertices, const Vector2* normals, uint32_t vertexCount, uint32_t segments, Effect effect, Tr2DebugColor color )
{
	if( segments < 3 )
	{
		segments = 3;
	}

	auto Coords = [&]( float alpha, const Vector2& e )->Vector3
	{
		Vector3 v;
		v.y = e.x;
		v.x = sin( alpha ) * e.y;
		v.z = cos( alpha ) * e.y;
		return v;
	};

	for( uint32_t j = 0; j + 1 < vertexCount; ++j )
	{
		const Vector2& e0 = vertices[j];
		const Vector2& e1 = vertices[j + 1];
		for( uint32_t i = 0; i < segments; ++i )
		{
			float alpha0 = float( i ) / float( segments ) * 2 * XM_PI;
			float alpha1 = float( i + 1 ) / float( segments ) * 2 * XM_PI;
			
			Vector3 p00( XMVector3TransformCoord( Coords( alpha0, e0 ), transform ) );
			Vector3 p10( XMVector3TransformCoord( Coords( alpha1, e0 ), transform ) );
			Vector3 p01( XMVector3TransformCoord( Coords( alpha0, e1 ), transform ) );
			Vector3 p11( XMVector3TransformCoord( Coords( alpha1, e1 ), transform ) );

			switch( effect )
			{
			case Wireframe:
				DrawLine( owner, p00, p10, color );
				DrawLine( owner, p00, p01, color );
				DrawLine( owner, p01, p11, color );
				break;
			case Solid:
				DrawTriangle( owner, p00, p10, p01, Vector3( 0, 0, 0 ), color );
				DrawTriangle( owner, p10, p11, p01, Vector3( 0, 0, 0 ), color );
				break;
			default:
				const Vector2& n0 = normals[j * 2];
				const Vector2& n1 = normals[j * 2 + 1];

				Vector3 n00( XMVector3TransformNormal( Coords( alpha0, n0 ), transform ) );
				Vector3 n10( XMVector3TransformNormal( Coords( alpha1, n0 ), transform ) );
				Vector3 n01( XMVector3TransformNormal( Coords( alpha0, n1 ), transform ) );
				Vector3 n11( XMVector3TransformNormal( Coords( alpha1, n1 ), transform ) );

				DrawTriangle( owner, p00, n00, p10, n10, p01, n01, color );
				DrawTriangle( owner, p10, n10, p11, n11, p01, n01, color );
			}
		}
	}
}

void Tr2DebugRenderer::DrawText( TriDebugFont font, const Vector3& pos, const Color& color, const char* msg, ... )
{
	va_list args;
    va_start( args, msg );

	Tr2Rect rect;
	USE_MAIN_THREAD_RENDER_CONTEXT();
	Tr2Viewport vp;
	renderContext.GetViewport( vp );

	Vector3 screenPos = Tr2Renderer::ProjectWorldToScreen( pos, vp );

	if( (screenPos.z > 0.0f) && (screenPos.z < 1.0f) )
	{
		rect.top = (int32_t)screenPos.y;
		rect.left = (int32_t)screenPos.x;
		rect.bottom = rect.top + 512;
		rect.right = rect.left + 1024;

		GetDebugTextRenderer().Vprintf( font, rect, TRI_DFS_LEFT, Vector4( color.r, color.g, color.b, color.a ), msg, args );
	}
	va_end( args );
}

Tr2DebugObjectReference Tr2DebugRenderer::Pick( float& depth, Tr2RenderContext& renderContext )
{
	auto shader = m_pickingEffect->GetShaderStateInterface();
	if( !shader )
	{
		return nullptr;
	}
	if( m_objectLineOffsets.empty() && m_objectTriangleOffsets.empty() )
	{
		return nullptr;
	}

	if( !m_pickBuffer.BeginRendering( 0.f, renderContext ) )
	{
		return nullptr;
	}

	auto handle = GetVertexDeclarationHandle( renderContext );

	shader->ApplyAllStateForPass( 0, 0, renderContext );
	m_pickingEffect->ApplyMaterialDataForPass( 0, 0, renderContext );
	renderContext.m_esm.ApplyVertexDeclaration( handle );

	if( !m_objectLineOffsets.empty() )
	{
		renderContext.SetTopology( Tr2RenderContextEnum::TOP_LINES );

		for( size_t i = 0; i < m_objectLineOffsets.size(); ++i )
		{
			auto object = m_objectLineOffsets[i].first;
			if( !object )
			{
				continue;
			}
			auto begin = m_objectLineOffsets[i].second;
			auto end = i + 1 < m_objectLineOffsets.size() ? m_objectLineOffsets[i + 1].second : m_lines.size();
			renderContext.DrawPrimitiveUP( uint32_t( ( end - begin ) / 2 ), &m_lines[begin], uint32_t( sizeof( Vertex ) ) );
		}
	}

	if( !m_objectTriangleOffsets.empty() )
	{
		renderContext.SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES );

		for( size_t i = 0; i < m_objectTriangleOffsets.size(); ++i )
		{
			auto object = m_objectTriangleOffsets[i].first;
			if( !object )
			{
				continue;
			}
			auto begin = m_objectTriangleOffsets[i].second;
			auto end = i + 1 < m_objectTriangleOffsets.size() ? m_objectTriangleOffsets[i + 1].second : m_triangles.size();
			renderContext.DrawPrimitiveUP( uint32_t( ( end - begin ) / 3 ), &m_triangles[begin], uint32_t( sizeof( Vertex ) ) );
		}
	}

	if( !m_pickBuffer.EndRendering( renderContext ) )
	{
		return nullptr;
	}

	const void* data;
	uint32_t pitch;
	if( m_pickBuffer.PrepareGetResults( data, pitch, renderContext ) )
	{
		const float* pixels = static_cast<const float*>( data );
		uint32_t index = uint32_t( pixels[0] + 0.5f );
		bool isLine = pixels[1] != 0;
		depth = pixels[2];

		Tr2DebugObjectReference object = nullptr;

		if( isLine )
		{
			if( index > 0 && index <= m_objectLineOffsets.size() )
			{
				object = m_objectLineOffsets[index - 1].first;
			}
		}
		else
		{
			if( index > 0 && index <= m_objectTriangleOffsets.size() )
			{
				object = m_objectTriangleOffsets[index - 1].first;
			}
		}

		m_pickBuffer.UnlockBuffer( renderContext );
		return object;
	}
	return nullptr;
}

void Tr2DebugRenderer::BeginRender()
{
	m_lines.clear();
	m_triangles.clear();
	m_objectLineOffsets.clear();
	m_objectTriangleOffsets.clear();
}

void Tr2DebugRenderer::EndRender( Tr2RenderContext& renderContext )
{
	GetDebugTextRenderer().Render( renderContext );
	GetDebugTextRenderer().Clear();

	auto shader = m_effect->GetShaderStateInterface();
	if( !shader )
	{
		return;
	}
	if( m_lines.empty() && m_triangles.empty() )
	{
		return;
	}

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA );

	auto handle = GetVertexDeclarationHandle( renderContext );
	if( !m_lines.empty() )
	{
		renderContext.m_esm.ApplyVertexDeclaration( handle );
		renderContext.SetTopology( Tr2RenderContextEnum::TOP_LINES );

		uint32_t passCount = shader->GetPassCount( 0 );

		for( uint32_t passIx = 0; passIx < passCount; ++passIx )
		{
			shader->ApplyAllStateForPass( 0, passIx, renderContext );
			m_effect->ApplyMaterialDataForPass( 0, passIx, renderContext );
			renderContext.DrawPrimitiveUP( uint32_t( m_lines.size() / 2 ), &m_lines[0], uint32_t( sizeof( Vertex ) ) );
		}
	}
	if( !m_triangles.empty() )
	{
		renderContext.m_esm.ApplyVertexDeclaration( handle );
		renderContext.SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES );

		uint32_t passCount = shader->GetPassCount( 0 );

		for( uint32_t passIx = 0; passIx < passCount; ++passIx )
		{
			shader->ApplyAllStateForPass( 0, passIx, renderContext );
			m_effect->ApplyMaterialDataForPass( 0, passIx, renderContext );
			renderContext.DrawPrimitiveUP( uint32_t( m_triangles.size() / 3 ), &m_triangles[0], uint32_t( sizeof( Vertex ) ) );
		}
	}
}


void Tr2DebugRenderer::SetSelectedObjects( const std::vector<std::pair<IRoot*, uint32_t>>& objects )
{
	m_selectedObjects.clear();
	for( auto it = objects.begin(); it != objects.end(); ++it )
	{
		if( it->first )
		{
			m_selectedObjects.insert( Tr2DebugObjectReference( it->first, it->second ) );
		}
	}
}

void Tr2DebugRenderer::SetOptions( IRoot* owner, std::vector<Tr2DebugRendererOption>& options )
{
	if( options.empty() )
	{
		auto found = m_options.find( owner );
		if( found != m_options.end() )
		{
			m_options.erase( found );
		}
		return;
	}

	auto& dest = m_options[owner];
	dest.clear();
	dest.insert( options.begin(), options.end() );
}


bool Tr2DebugRenderer::GetColorForOption( Color& color, const Tr2DebugRendererOption& option ) const
{
	auto iter = m_optionColors.find( option );
	if( iter != m_optionColors.end() )
	{
		color = iter->second;
		return true;
	}
	return false;
}

void Tr2DebugRenderer::SetColorForOption( const Tr2DebugRendererOption& option, const Color& color )
{
	m_optionColors.insert_or_assign( option, color );
}

#if BLUE_WITH_PYTHON
PyObject* Tr2DebugRenderer::PyGetColorForOption( PyObject* self, PyObject* args )
{
	auto renderer = BluePythonCast<Tr2DebugRenderer*>( self );
	const char* option;
	if( !PyArg_ParseTuple( args, "s", &option ) )
	{
		return nullptr;
	}
	Color color;
	if( renderer->GetColorForOption( color, option ) )
	{
		PyObject* result = Py_BuildValue( "(ffff)", color.r, color.g, color.b, color.a );
		return result;
	}
	Py_RETURN_NONE;
}
#endif

std::vector<Tr2DebugRendererOption> Tr2DebugRenderer::GetOptions( IRoot* owner ) const
{
	std::vector<Tr2DebugRendererOption> result;
	auto found = m_options.find( owner );
	if( found != m_options.end() )
	{
		result.insert( result.end(), found->second.begin(), found->second.end() );
	}
	return result;
}

void Tr2DebugRenderer::SetDefaultOptions( const std::vector<Tr2DebugRendererOption>& options )
{
	m_defaultOptions.clear();
	m_defaultOptions.insert( options.begin(), options.end() );
}

std::vector<Tr2DebugRendererOption> Tr2DebugRenderer::GetDefaultOptions() const
{
	std::vector<Tr2DebugRendererOption> result;
	result.insert( result.end(), m_defaultOptions.begin(), m_defaultOptions.end() );
	return result;
}
