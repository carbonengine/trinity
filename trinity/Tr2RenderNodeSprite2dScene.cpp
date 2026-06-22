// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2RenderNodeSprite2dScene.h"
#include "Sprite2d/Tr2Sprite2dScene.h"


bool Tr2RenderNodeSprite2dScene::Validate( const Span<const Tr2BitmapDimensions>& destDimensions, const Span<const BlueSharedString>& outputs, Be::Time realTime, Be::Time simTime )
{
	if( destDimensions.size == 0 )
	{
		CCP_ASSERT_M( false, "Tr2RenderNodeSprite2dScene requires at least one destination texture" );
		return false;
	}
	if( !m_scene )
	{
		return false;
	}
	if( m_background )
	{
		if( !m_background->Validate( destDimensions, {}, realTime, simTime ) )
		{
			return false;
		}
	}
	return true;
}

void Tr2RenderNodeSprite2dScene::Execute( const Span<const Tr2TextureAL>& destinations, const Span<TempOutput>& outputs, Be::Time realTime, Be::Time simTime, const Tr2ProfileTimer& rootTimer, Tr2RenderContext& renderContext )
{
	if( m_background )
	{
		m_background->Execute( destinations, {}, realTime, simTime, rootTimer, renderContext );
	}
	renderContext.m_esm.SetRenderTarget( 0, destinations[0] );
	renderContext.m_esm.SetDepthStencilBuffer( {} );
	renderContext.m_esm.SetFullScreenViewport();
	m_scene->Update( realTime, simTime );
	m_scene->Render( renderContext );
}
