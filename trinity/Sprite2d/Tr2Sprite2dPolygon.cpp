// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dPolygon.h"
#include "Tr2Sprite2dTexture.h"
#include "Tr2Sprite2dScene.h"
#include "Tr2Sprite2dPickingMask.h"

Tr2Sprite2dPolygon::Tr2Sprite2dPolygon( IRoot* lockobj ) :
	PARENTLOCK( m_vertices ),
	PARENTLOCK( m_triangles )
{
	m_vertices.SetNotify( this );
	m_triangles.SetNotify( this );
}

Tr2Sprite2dPolygon::~Tr2Sprite2dPolygon()
{
}

void Tr2Sprite2dPolygon::GatherSprites( Tr2Sprite2dScene* renderer )
{
	if( m_isDirty && m_display && !m_triangles.empty() )
	{
		if( !ValidateAndSetTextures( renderer ) )
		{
			return;
		}

		renderer->SetSpriteEffect( m_spriteEffect );
		renderer->SetTileMode( 0 );

		SetRegularRenderState( renderer );

		size_t count = m_vertices.size();
		std::vector<Tr2Sprite2dVertexBase> verts( count );
		for( size_t i = 0; i < count; ++i )
		{
			Tr2Sprite2dVertexBase& source = *m_vertices[i];
			Tr2Sprite2dVertexBase& dest = verts[i];

			dest.position = source.position;
			dest.color = source.color;
			dest.texCoord[0] = source.texCoord[0];
			dest.texCoord[1] = source.texCoord[1];
		}

		auto maxTriangles = std::min( renderer->GetMaxIndexCountPerDrawCall() / 3, renderer->GetMaxVertexCountPerDrawCall() / 3 );
		m_indices.resize( ( m_triangles.size() + maxTriangles - 1 ) / maxTriangles );
		m_renderVertices.resize( m_indices.size() );

		if( m_indices.size() == 1 )
		{
			auto& indices = m_indices[0];
			indices.resize( m_triangles.size() * 3 );
			int ix = 0;
			for( Tr2Sprite2dTriangleVector::iterator it = m_triangles.begin(); it != m_triangles.end(); ++it )
			{
				indices[ix++] = ( *it )->m_index[0];
				indices[ix++] = ( *it )->m_index[1];
				indices[ix++] = ( *it )->m_index[2];
			}

			m_renderVertices[0].resize( m_vertices.size() );
			renderer->PrepareTriangleVerts( &m_renderVertices[0][0], &verts[0], sizeof( Tr2Sprite2dVertexBase ), unsigned( m_vertices.size() ) );
		}
		else
		{
			const uint16_t INVALID_INDEX = 0xffff;
			Tr2Sprite2dTriangleVector::iterator it = m_triangles.begin();
			std::vector<uint16_t> indexMap( m_triangles.size() * 3 );
			std::vector<Tr2Sprite2dVertexBase> localVerts;

			for( size_t i = 0; i < m_indices.size(); ++i )
			{
				std::fill( indexMap.begin(), indexMap.end(), INVALID_INDEX );
				localVerts.clear();

				auto& indices = m_indices[i];
				indices.reserve( maxTriangles * 3 );
				indices.clear();
				while( it != m_triangles.end() && indices.size() / 3 < maxTriangles )
				{
					for( uint32_t j = 0; j < 3; ++j )
					{
						if( indexMap[( *it )->m_index[j]] == INVALID_INDEX )
						{
							localVerts.push_back( verts[( *it )->m_index[j]] );
							indexMap[( *it )->m_index[j]] = uint16_t( localVerts.size() ) - 1;
						}
						indices.push_back( indexMap[( *it )->m_index[j]] );
					}
					++it;
				}
				m_renderVertices[i].resize( localVerts.size() );
				renderer->PrepareTriangleVerts( &m_renderVertices[i][0], &localVerts[0], sizeof( Tr2Sprite2dVertexBase ), unsigned( localVerts.size() ) );
			}
		}
		m_isDirty = false;
	}

	if( !m_isDirty )
	{
		SetValidatedTextures( renderer );

		renderer->PushTranslation( m_translation );
		for( size_t i = 0; i < m_indices.size(); ++i )
		{
			renderer->RenderTriangleVerts( &m_renderVertices[i][0], unsigned( m_renderVertices[i].size() ), &m_indices[i][0], uint16_t( m_indices[i].size() ) );
		}
		renderer->PopTranslation();
	}
}

ITr2SpriteObject* Tr2Sprite2dPolygon::PickPoint( float x, float y, Tr2Sprite2dScene* renderer )
{
	if( !m_display || m_pickState != TR2_SPS_ON )
	{
		return nullptr;
	}

	if( !m_pickingMask || m_pickingMask->SampleMask( renderer->InverseTransformPoint( Vector2( x, y ) ), m_translation, m_displayWidth, m_displayHeight ) )
	{
		renderer->PushTranslation( m_translation );
		ON_BLOCK_EXIT( [renderer] { renderer->PopTranslation(); } );

		Vector2 query( x, y );

		for( auto it = m_triangles.begin(); it != m_triangles.end(); ++it )
		{
			auto p0 = m_vertices[( *it )->m_index[0]]->position;
			auto p1 = m_vertices[( *it )->m_index[1]]->position;
			auto p2 = m_vertices[( *it )->m_index[2]]->position;

			if( renderer->IsInsideTriangle( query, Vector2( p0.x, p0.y ), Vector2( p1.x, p1.y ), Vector2( p2.x, p2.y ) ) )
			{
				return this;
			}
			if( renderer->IsInsideTriangle( query, Vector2( p0.x, p0.y ), Vector2( p2.x, p2.y ), Vector2( p1.x, p1.y ) ) )
			{
				return this;
			}
		}
	}
	return nullptr;
}

Color Tr2Sprite2dPolygon::GetColor() const
{
	return m_color;
}

void Tr2Sprite2dPolygon::SetColor( Color val )
{
	m_color = val;
}

void Tr2Sprite2dPolygon::OnListModified( long event, /* BLUELISTEVENT values */ ssize_t key, ssize_t key2, IRoot* value, const IList* theList )
{
	SetDirty();
}

unsigned int Tr2Sprite2dPolygon::GetVertexCount()
{
	if( !m_display )
	{
		return 0;
	}

	return (unsigned int)m_vertices.size();
}

Tr2Sprite2dVertex::Tr2Sprite2dVertex( IRoot* lockobj /*= NULL */ )
{
	position = Vector3( 0.0f, 0.0f, 0.0f );
	color = Color( 1.0f, 1.0f, 1.0f, 1.0f );
	texCoord[0] = Vector2( 0.0f, 0.0f );
	texCoord[1] = Vector2( 0.0f, 0.0f );
}


void Tr2Sprite2dVertex::SetTexCoord( unsigned i, Vector2 uv )
{
	CCP_ASSERT( i < 2 );
	texCoord[i] = uv;
}
Vector2 Tr2Sprite2dVertex::GetTexCoord( unsigned i ) const
{
	CCP_ASSERT( i < 2 );
	return texCoord[i];
}

void Tr2Sprite2dVertex::SetColor( Color c )
{
	color = c;
}
Color Tr2Sprite2dVertex::GetColor() const
{
	return color;
}

Tr2Sprite2dTriangle::Tr2Sprite2dTriangle( IRoot* lockobj /*= NULL */ )
{
	m_index[0] = 0;
	m_index[1] = 0;
	m_index[2] = 0;
}