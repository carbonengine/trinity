// Copyright © 2012 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dArc.h"
#include "Tr2Sprite2dTexture.h"
#include "Tr2Sprite2dScene.h"

Tr2Sprite2dArc::Tr2Sprite2dArc( IRoot* lockobj /*= NULL */ ) :
	m_radius( 0.0f ),
	m_startAngle( 0.0f ),
	m_endAngle( 0.0f ),
	m_lineWidth( 1.0f ),
	m_lineColor( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_fill( false ),
	m_renderVertices( nullptr ),
	m_renderVertexCapacity( 0 ),
	m_renderVertexCount( 0 ),
	m_renderIndices( "Tr2Sprite2dArc/m_renderIndices" )
{
}

Tr2Sprite2dArc::~Tr2Sprite2dArc()
{
	ClearVertices();
}

unsigned int Tr2Sprite2dArc::GetVertexCount()
{
	unsigned int numVerts = 0;

	float arcLength = ( m_endAngle - m_startAngle ) * m_radius;
	unsigned int numSteps = (unsigned int)arcLength / 4;
	if( m_lineWidth > 0 )
	{
		numVerts += numSteps * 2 + 2;
	}

	if( m_fill )
	{
		numVerts += 1 + numSteps + 1;

		// Add room for anti-aliasing
		numVerts += 8;
	}

	return numVerts;
}

void Tr2Sprite2dArc::GatherSprites( Tr2Sprite2dScene* renderer )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_display || ( m_spriteEffect == TR2_SFX_NONE ) || ( m_radius == 0.0f ) || ( m_startAngle >= m_endAngle ) )
	{
		if( m_renderVertices )
		{
			ClearVertices();
		}
		return;
	}

	if( m_isDirty )
	{
		ClearVertices();

		if( m_spriteEffect >= TR2_SFX_ONE_TEXTURE )
		{
			if( m_texturePrimary )
			{
				if( !m_texturePrimary->IsGood() )
				{
					// Don't draw if we're still loading (or failed)
					return;
				}
				m_texturePrimary->Apply( renderer, 0 );
			}
			else
			{
				// Don't draw if no texture set
				return;
			}

			if( m_spriteEffect >= TR2_SFX_TWO_TEXTURES )
			{
				if( m_textureSecondary )
				{
					if( !m_textureSecondary->IsGood() )
					{
						// Don't draw if we're still loading (or failed)
						return;
					}
					m_textureSecondary->Apply( renderer, 1 );
				}
				else
				{
					// Don't draw if no texture set
					return;
				}
			}
		}

		// Anti-aliased lines are rendered with a quad that is larger. This is then
		// compensated for in the pixel shader, using the extra pixels to fill in
		// alpha values to do the anti-aliasing

		bool isAA = m_spriteEffect == TR2_SFX_FILL_AA;
		renderer->SetSpriteEffect( m_spriteEffect );
		renderer->SetTileMode( 0 );

		SetRegularRenderState( renderer );

		m_renderVertexCapacity = GetVertexCount();
		m_renderVertices = static_cast<Tr2Sprite2dD3DVertex*>( CCP_MALLOC(
			"Tr2Sprite2dArc/m_renderVertices",
			m_renderVertexCapacity * sizeof( Tr2Sprite2dD3DVertex ) ) );
		m_renderVertexCount = 0;

		float arcLength = ( m_endAngle - m_startAngle ) * m_radius;
		unsigned int numSteps = (unsigned int)arcLength / 4;
		float stepSize = ( m_endAngle - m_startAngle ) / (float)numSteps;
		++numSteps;

		if( m_lineWidth > 0.0f )
		{
			float halfWidth = m_lineWidth * 0.5f;
			float outerRadius = m_radius + halfWidth;
			float innerRadius = m_radius - halfWidth;
			float pixelWidthInTexels;
			if( isAA )
			{
				innerRadius -= 2.0f;
				outerRadius += 2.0f;
				pixelWidthInTexels = 1.0f / ( m_lineWidth + 4.0f );
			}

			float a = m_startAngle;

			Tr2Sprite2dVertexBase verts[2];
			for( unsigned int i = 0; i < numSteps; ++i )
			{
				// TODO: UV coordinates

				Tr2Sprite2dVertexBase& v0 = verts[0];
				v0.position.x = cos( a ) * outerRadius + m_translation.x;
				v0.position.y = sin( a ) * outerRadius + m_translation.y;
				v0.position.z = m_depth;
				v0.color = m_lineColor;

				Tr2Sprite2dVertexBase& v1 = verts[1];
				v1.position.x = cos( a ) * innerRadius + m_translation.x;
				v1.position.y = sin( a ) * innerRadius + m_translation.y;
				v1.position.z = m_depth;
				v1.color = m_lineColor;

				if( isAA )
				{
					v0.texCoord[1] = Vector2( 1.0f + pixelWidthInTexels, m_lineWidth );
					v1.texCoord[1] = Vector2( -pixelWidthInTexels, m_lineWidth );
				}
				renderer->PrepareTriangleVerts(
					m_renderVertices + m_renderVertexCount,
					verts,
					sizeof( Tr2Sprite2dVertexBase ),
					2 );

				if( i < numSteps - 1 )
				{
					m_renderIndices.push_back( 0 + m_renderVertexCount );
					m_renderIndices.push_back( 1 + m_renderVertexCount );
					m_renderIndices.push_back( 2 + m_renderVertexCount );
					m_renderIndices.push_back( 2 + m_renderVertexCount );
					m_renderIndices.push_back( 3 + m_renderVertexCount );
					m_renderIndices.push_back( 1 + m_renderVertexCount );
				}
				m_renderVertexCount += 2;
				a += stepSize;

				CCP_ASSERT( m_renderVertexCount <= m_renderVertexCapacity );
			}
		}

		if( m_fill )
		{
			float halfWidth = m_lineWidth * 0.5f;
			float innerRadius = m_radius - halfWidth;

			float startAngle = m_startAngle;
			float endAngle = m_endAngle;

			float pixelWidthInTexels;
			if( isAA )
			{
				innerRadius += 2.0f;
				pixelWidthInTexels = 1.0f / ( innerRadius + 4.0f );

				float angleDelta = 0.5f / m_radius;
				startAngle += angleDelta * 0.5f;
				endAngle -= angleDelta * 0.5f;
				arcLength = ( endAngle - startAngle ) * m_radius;
				numSteps = (unsigned int)arcLength / 4;
				stepSize = ( endAngle - startAngle ) / (float)numSteps;
				++numSteps;
			}

			float a = startAngle;

			unsigned int fillVertCount = 1 + numSteps;
			if( isAA )
			{
				fillVertCount += 8;
			}
			Tr2Sprite2dVertexBase* verts = static_cast<Tr2Sprite2dVertexBase*>( CCP_MALLOC(
				"Tr2Sprite2dArc/verts",
				fillVertCount * sizeof( Tr2Sprite2dVertexBase ) ) );

			unsigned int centerPointIx = m_renderVertexCount;

			Tr2Sprite2dVertexBase* currentVertex = &verts[0];

			// Center point
			Tr2Sprite2dVertexBase& v0 = *currentVertex++;
			v0.position.x = m_translation.x;
			v0.position.y = m_translation.y;
			v0.position.z = m_depth;
			v0.color = m_color;

			if( isAA )
			{
				v0.texCoord[1] = Vector2( 0.0f, m_radius - halfWidth );
			}

			m_renderVertexCount += 1;

			if( isAA )
			{
				{
					Vector2 from = m_translation;
					Vector2 to;
					to.x = cos( m_startAngle ) * ( innerRadius - 2.0f ) + m_translation.x;
					to.y = sin( m_startAngle ) * ( innerRadius - 2.0f ) + m_translation.y;

					Vector2 d = Normalize( from - to );

					// Rotate 90 degrees
					Vector2 normal;
					normal.x = d.y;
					normal.y = -d.x;

					float halfWidth = 1.5f;

					Vector2 aaOffset = normal * 2.0f;
					float pixelWidthInTexels = 1.0f / ( 1.0f + 4.0f );

					Tr2Sprite2dVertexBase& v0 = *currentVertex++;
					v0.position.x = from.x - normal.x * halfWidth - aaOffset.x;
					v0.position.y = from.y - normal.y * halfWidth - aaOffset.y;
					v0.position.z = m_depth;
					v0.color = m_color;
					v0.texCoord[1] = Vector2( -pixelWidthInTexels, 1.0f );

					Tr2Sprite2dVertexBase& v1 = *currentVertex++;
					v1.position.x = from.x + normal.x * halfWidth + aaOffset.x;
					v1.position.y = from.y + normal.y * halfWidth + aaOffset.y;
					v1.position.z = m_depth;
					v1.color = m_color;
					v1.texCoord[1] = Vector2( 1.0f + pixelWidthInTexels, 1.0f );

					Tr2Sprite2dVertexBase& v2 = *currentVertex++;
					v2.position.x = to.x + normal.x * halfWidth + aaOffset.x;
					v2.position.y = to.y + normal.y * halfWidth + aaOffset.y;
					v2.position.z = m_depth;
					v2.color = m_color;
					v2.texCoord[1] = Vector2( 1.0f + pixelWidthInTexels, 1.0f );

					Tr2Sprite2dVertexBase& v3 = *currentVertex++;
					v3.position.x = to.x - normal.x * halfWidth - aaOffset.x;
					v3.position.y = to.y - normal.y * halfWidth - aaOffset.y;
					v3.position.z = m_depth;
					v3.color = m_color;
					v3.texCoord[1] = Vector2( -pixelWidthInTexels, 1.0f );

					m_renderIndices.push_back( 0 + m_renderVertexCount );
					m_renderIndices.push_back( 1 + m_renderVertexCount );
					m_renderIndices.push_back( 3 + m_renderVertexCount );
					m_renderIndices.push_back( 3 + m_renderVertexCount );
					m_renderIndices.push_back( 1 + m_renderVertexCount );
					m_renderIndices.push_back( 2 + m_renderVertexCount );

					m_renderVertexCount += 4;
					CCP_ASSERT( m_renderVertexCount <= m_renderVertexCapacity );
				}
				{
					Vector2 from = m_translation;
					Vector2 to;
					to.x = cos( m_endAngle ) * ( innerRadius - 2.0f ) + m_translation.x;
					to.y = sin( m_endAngle ) * ( innerRadius - 2.0f ) + m_translation.y;

					Vector2 d = Normalize( from - to );

					// Rotate 90 degrees
					Vector2 normal;
					normal.x = d.y;
					normal.y = -d.x;

					float halfWidth = 1.5f;

					Vector2 aaOffset = normal * 2.0f;
					float pixelWidthInTexels = 1.0f / ( 1.0f + 4.0f );

					Tr2Sprite2dVertexBase& v0 = *currentVertex++;
					v0.position.x = from.x - normal.x * halfWidth - aaOffset.x;
					v0.position.y = from.y - normal.y * halfWidth - aaOffset.y;
					v0.position.z = m_depth;
					v0.color = m_color;
					v0.texCoord[1] = Vector2( -pixelWidthInTexels, 1.0f );

					Tr2Sprite2dVertexBase& v1 = *currentVertex++;
					v1.position.x = from.x + normal.x * halfWidth + aaOffset.x;
					v1.position.y = from.y + normal.y * halfWidth + aaOffset.y;
					v1.position.z = m_depth;
					v1.color = m_color;
					v1.texCoord[1] = Vector2( 1.0f + pixelWidthInTexels, 1.0f );

					Tr2Sprite2dVertexBase& v2 = *currentVertex++;
					v2.position.x = to.x + normal.x * halfWidth + aaOffset.x;
					v2.position.y = to.y + normal.y * halfWidth + aaOffset.y;
					v2.position.z = m_depth;
					v2.color = m_color;
					v2.texCoord[1] = Vector2( 1.0f + pixelWidthInTexels, 1.0f );

					Tr2Sprite2dVertexBase& v3 = *currentVertex++;
					v3.position.x = to.x - normal.x * halfWidth - aaOffset.x;
					v3.position.y = to.y - normal.y * halfWidth - aaOffset.y;
					v3.position.z = m_depth;
					v3.color = m_color;
					v3.texCoord[1] = Vector2( -pixelWidthInTexels, 1.0f );

					m_renderIndices.push_back( 0 + m_renderVertexCount );
					m_renderIndices.push_back( 1 + m_renderVertexCount );
					m_renderIndices.push_back( 3 + m_renderVertexCount );
					m_renderIndices.push_back( 3 + m_renderVertexCount );
					m_renderIndices.push_back( 1 + m_renderVertexCount );
					m_renderIndices.push_back( 2 + m_renderVertexCount );

					m_renderVertexCount += 4;
					CCP_ASSERT( m_renderVertexCount <= m_renderVertexCapacity );
				}
			}

			for( unsigned int i = 0; i < numSteps; ++i )
			{
				// TODO: UV coordinates

				Tr2Sprite2dVertexBase& v = *currentVertex++;
				v.position.x = cos( a ) * innerRadius + m_translation.x;
				v.position.y = sin( a ) * innerRadius + m_translation.y;
				v.position.z = m_depth;
				v.color = m_color;

				if( isAA )
				{
					v.texCoord[1] = Vector2( 1.0f + pixelWidthInTexels, m_radius - halfWidth );
				}

				if( i > 0 )
				{
					m_renderIndices.push_back( -1 + m_renderVertexCount );
					m_renderIndices.push_back( 0 + m_renderVertexCount );
					m_renderIndices.push_back( centerPointIx );
				}

				m_renderVertexCount += 1;
				a += stepSize;

				CCP_ASSERT( m_renderVertexCount <= m_renderVertexCapacity );
			}

			renderer->PrepareTriangleVerts(
				m_renderVertices + centerPointIx,
				verts,
				sizeof( Tr2Sprite2dVertexBase ),
				fillVertCount );

			CCP_FREE( verts );
		}

		m_isDirty = false;
	}

	renderer->RenderTriangleVerts( m_renderVertices, m_renderVertexCount, &m_renderIndices[0], (unsigned short)m_renderIndices.size() );
}

ITr2SpriteObject* Tr2Sprite2dArc::PickPoint( float x, float y, Tr2Sprite2dScene* renderer )
{
	return nullptr;
}

void Tr2Sprite2dArc::ClearVertices()
{
	CCP_FREE( m_renderVertices );
	m_renderVertices = nullptr;
	m_renderVertexCapacity = 0;
	m_renderVertexCount = 0;
	m_renderIndices.clear();
	m_renderIndices.shrink_to_fit();
}
