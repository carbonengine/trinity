// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepRenderTexture.h"
#include "Tr2Renderer.h"
#include "Tr2AtlasTexture.h"
#include "Tr2RenderTarget.h"
#include "Tr2DepthStencil.h"
#include "Resources/TriTextureRes.h"

using namespace Tr2RenderContextEnum;

TriStepRenderTexture::TriStepRenderTexture( IRoot* lockobj ) :
	m_textureSize( 0, 0 ),
	m_failClearColor( 0 ),
	m_tlTexCoord( 0.0f, 0.0f ),
	m_brTexCoord( 1.0f, 1.0f )
{
}

TriStepRenderTexture::~TriStepRenderTexture( void )
{
}

void TriStepRenderTexture::BlankOut()
{
	m_texture.Unlock();
	m_atlasTexture = nullptr;
}

void TriStepRenderTexture::SetTexture( ITr2TextureProvider* tex )
{
	BlankOut();
	m_texture = tex;
}

void TriStepRenderTexture::SetTexture( Tr2AtlasTexture* tex )
{
	BlankOut();
	m_atlasTexture = tex;
}

TriStepResult TriStepRenderTexture::ClearIfFail( bool result, Tr2RenderContext& renderContext )
{
	if( !result )
	{
		renderContext.Clear( CLEARFLAGS_TARGET, m_failClearColor, 0, 0 );
	}
	return RS_OK;
}

TriStepResult TriStepRenderTexture::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( m_texture && m_texture->GetTexture() )
	{
		return ClearIfFail( Tr2Renderer::DrawTexture( renderContext, *m_texture->GetTexture(), m_tlTexCoord, m_brTexCoord ), renderContext );
	}

	if( m_atlasTexture && m_atlasTexture->GetTexture() )
	{
		float rectX = m_tlTexCoord.x * m_atlasTexture->GetWidth();
		float rectY = m_tlTexCoord.y * m_atlasTexture->GetHeight();
		float rectWidth = ( m_brTexCoord.x - m_tlTexCoord.x ) * m_atlasTexture->GetWidth();
		float rectHeight = ( m_brTexCoord.y - m_tlTexCoord.y ) * m_atlasTexture->GetHeight();

		Vector4 tw;
		m_atlasTexture->CalcSubTextureWindow( tw, rectX, rectY, rectWidth, rectHeight );

		return ClearIfFail( Tr2Renderer::DrawTexture( renderContext, *m_texture->GetTexture(), Vector2( tw.x, tw.y ), Vector2( tw.x + tw.z, tw.y + tw.w ) ), renderContext );
	}

	return RS_OK;
}
