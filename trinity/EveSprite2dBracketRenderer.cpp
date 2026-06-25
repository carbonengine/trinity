// Copyright © 2012 CCP ehf.

#include "StdAfx.h"
#include "EveSprite2dBracketRenderer.h"
#include "EveSprite2dBracket.h"
#include "Tr2AtlasTexture.h"
#include "Sprite2d/Tr2Sprite2dScene.h"

EveSprite2dBracketRenderer::EveSprite2dBracketRenderer( IRoot* lockobj /*= nullptr */ ) :
	PARENTLOCK( m_brackets ),
	m_bracketCountInBuffers( 0 )
{
}

unsigned int EveSprite2dBracketRenderer::GetVertexCount()
{
	return 0;
}

void EveSprite2dBracketRenderer::GatherSprites( Tr2Sprite2dScene* renderer )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	USE_MAIN_THREAD_RENDER_CONTEXT();
	using namespace Tr2RenderContextEnum;

	if( !m_display )
	{
		return;
	}

	if( m_brackets.empty() )
	{
		return;
	}

	unsigned int vertexCount = (unsigned int)m_brackets.size() * 4;
	unsigned int indexCount = (unsigned int)m_brackets.size() * 6;

	Tr2Sprite2dD3DVertex* vertices;

	if( m_bracketCountInBuffers != m_brackets.size() )
	{
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			// Allocate new vertex/index buffers
			if( FAILED( m_vertexBuffer.Create( sizeof( Tr2Sprite2dD3DVertex ), vertexCount, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::READ | Tr2CpuUsage::WRITE, nullptr, renderContext ) ) )
			{
				CCP_LOGERR( "%s failed to create vertex buffer", __FUNCTION__ );
				return;
			}

			if( FAILED( m_indexBuffer.Create( 2, indexCount, Tr2GpuUsage::INDEX_BUFFER, Tr2CpuUsage::READ | Tr2CpuUsage::WRITE, nullptr, renderContext ) ) )
			{
				CCP_LOGERR( "%s failed to create index buffer", __FUNCTION__ );
				return;
			}
		}

		m_bracketCountInBuffers = (unsigned int)m_brackets.size();

		// Fill in the parts of the vertex buffer that don't change
		HRESULT hr = m_vertexBuffer.MapForWriting( vertices, renderContext );
		if( FAILED( hr ) )
		{
			CCP_LOGERR( "%s failed to lock vertex buffer (%d)", __FUNCTION__, hr );
			return;
		}

		// Fill the index buffer now - it only changes when the count changes.
		unsigned short* indices;
		hr = m_indexBuffer.MapForWriting( indices, renderContext );
		if( FAILED( hr ) )
		{
			CCP_LOGERR( "%s failed to lock index buffer (%d)", __FUNCTION__, hr );
			return;
		}
		ON_BLOCK_EXIT( [&] { m_indexBuffer.UnmapForWriting( renderContext ); } );

		unsigned short* curIndex = indices;
		unsigned int spriteIx = 0;

		for( unsigned int i = 0; i < m_bracketCountInBuffers; ++i )
		{
			for( unsigned int i = 0; i < 4; ++i )
			{
				Tr2Sprite2dD3DVertex& v = vertices[spriteIx * 4 + i];

				v.glowBrightness = 1;
				v.blendMode = PackBlendMode( TR2_SBM_BLEND, Tr2SpriteTarget::COLOR );
				v.spriteEffect = TR2_SFX_COPY;
				v.transformIndex = 0;
				v.clipRect = renderer->GetClipRectangle();
				v.tileMode = 0;
			}

			int vertexIx = spriteIx * 4;
			curIndex[0] = vertexIx;
			curIndex[1] = vertexIx + 1;
			curIndex[2] = vertexIx + 3;
			curIndex[3] = vertexIx + 3;
			curIndex[4] = vertexIx + 1;
			curIndex[5] = vertexIx + 2;

			++spriteIx;
			curIndex += 6;
		}
	}
	else
	{
		auto hr = m_vertexBuffer.MapForWriting( vertices, renderContext );
		if( FAILED( hr ) )
		{
			CCP_LOGERR( "%s failed to lock vertex buffer", __FUNCTION__ );
			return;
		}
	}

	m_iconAtlas.Unlock();

	Tr2Sprite2dD3DVertex* curVertex = vertices;

	Vector2 baseTranslation = renderer->GetTranslation() + m_translation;

	for( auto it = m_brackets.begin(); it != m_brackets.end(); ++it )
	{
		EveSprite2dBracket* bracket = *it;

		if( !bracket->IsDisplay() )
		{
			vertexCount -= 4;
			indexCount -= 6;
			continue;
		}

		Tr2AtlasTexture* at = bracket->GetIcon();
		CCP_ASSERT( at );

		if( at->IsLoading() )
		{
			vertexCount -= 4;
			indexCount -= 6;
			continue;
		}

		if( !m_iconAtlas )
		{
			m_iconAtlas = at;
		}

		// All icons must come from the same texture atlas
		CCP_ASSERT( at->GetTexture() == m_iconAtlas->GetTexture() );

		Vector4 tw;
		at->GetTextureWindow( tw );

		Color color = bracket->GetColor();

		float width = (float)at->GetWidth();
		float height = (float)at->GetHeight();

		float xZero = tw.x;
		float yZero = tw.y;
		float xOne = xZero + tw.z;
		float yOne = yZero + tw.w;

		Vector2 translation = baseTranslation + bracket->GetTranslation();

		Tr2Sprite2dD3DVertex& tl = curVertex[0];
		tl.position.x = translation.x;
		tl.position.y = translation.y;
		tl.texCoord[0] = Vector2( xZero, yZero );
		tl.color = color;

		Tr2Sprite2dD3DVertex& tr = curVertex[1];
		tr.position.x = translation.x + width;
		tr.position.y = translation.y;
		tr.texCoord[0] = Vector2( xOne, yZero );
		tr.color = color;

		Tr2Sprite2dD3DVertex& br = curVertex[2];
		br.position.x = translation.x + width;
		br.position.y = translation.y + height;
		br.texCoord[0] = Vector2( xOne, yOne );
		br.color = color;

		Tr2Sprite2dD3DVertex& bl = curVertex[3];
		bl.position.x = translation.x;
		bl.position.y = translation.y + height;
		bl.texCoord[0] = Vector2( xZero, yOne );
		bl.color = color;

		curVertex += 4;
	}

	m_vertexBuffer.UnmapForWriting( renderContext );

	renderer->SetTexture( 0, m_iconAtlas, S2D_TS_NONE );
	renderer->RenderTriangleVerts( m_vertexBuffer, vertexCount, m_indexBuffer, indexCount );
}

ITr2SpriteObject* EveSprite2dBracketRenderer::PickPoint( float x, float y, Tr2Sprite2dScene* renderer )
{
	return nullptr;
}

void EveSprite2dBracketRenderer::ReleaseResources( TriStorage s )
{
	m_vertexBuffer = Tr2BufferAL();
	m_indexBuffer = Tr2BufferAL();
	m_bracketCountInBuffers = 0;
}

bool EveSprite2dBracketRenderer::OnPrepareResources()
{
	return true;
}
