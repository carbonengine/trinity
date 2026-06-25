// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dLayer.h"
#include "Tr2Sprite2dScene.h"

using namespace Tr2RenderContextEnum;

CCP_STATS_DECLARE( spriteSceneLayerMemoryUse, "Trinity/SpriteScene/LayerMemoryUse", false, CST_MEMORY, "Memory used for layer render targets" );

Tr2Sprite2dLayer::Tr2Sprite2dLayer( IRoot* lockobj ) :
	Tr2Sprite2dContainer( lockobj ),
	m_backgroundColor( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_color( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_clearBackground( true ),
	m_spriteEffect( TR2_SFX_COPY ),
	m_blendMode( TR2_SBM_NONE )
{
}

Tr2Sprite2dLayer::~Tr2Sprite2dLayer()
{
	FreeRenderTarget();
	if( m_textureSecondary )
	{
		m_textureSecondary->UnregisterForChangeNotification( this );
	}
}

void Tr2Sprite2dLayer::SetChildDirty( ITr2SpriteObject* child )
{
	m_isDirty = true;
	if( m_parent )
	{
		m_parent->SetChildDirty( this );
	}
}

void Tr2Sprite2dLayer::GatherSprites( Tr2Sprite2dScene* renderer )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !m_display )
	{
		return;
	}

	AllocateRenderTarget();

	if( !m_renderTarget.IsValid() )
	{
		return;
	}

	renderer->StartLayer( m_renderTarget );

	float oldOpacity = renderer->GetAccumulatedAlpha();
	renderer->SetAccumulatedAlpha( m_opacity * oldOpacity );

	if( m_isDirty )
	{
		if( m_clearBackground )
		{
			renderContext.Clear( CLEARFLAGS_TARGET, m_backgroundColor, 0, 0 );
		}

		for( ITr2SpriteObjectVector::reverse_iterator it = m_background.rbegin(); it != m_background.rend(); ++it )
		{
			( *it )->GatherSprites( renderer );
		}
		for( ITr2SpriteObjectVector::reverse_iterator it = m_children.rbegin(); it != m_children.rend(); ++it )
		{
			( *it )->GatherSprites( renderer );
		}

		m_isDirty = false;
	}

	renderer->SetColor( m_color );
	renderer->SetSpriteEffect( m_spriteEffect );
	renderer->SetBlendmode( m_blendMode );
	renderer->EndLayer( m_translation.x, m_translation.y, m_displayWidth, m_displayHeight, m_textureSecondary );

	renderer->SetAccumulatedAlpha( oldOpacity );
}

bool Tr2Sprite2dLayer::OnPrepareResources()
{
	if( m_renderTarget.IsValid() )
	{
		return true;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();
	HRESULT hr = m_renderTarget.Create(
		Tr2BitmapDimensions( (uint32_t)m_displayWidth, (uint32_t)m_displayHeight, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ),
		Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::RENDER_TARGET,
		Tr2CpuUsage::READ,
		renderContext );

	if( FAILED( hr ) )
	{
		CCP_LOGERR( "Tr2Sprite2dLayer::AllocateRenderTarget failed to create a render target" );
		return false;
	}

	CCP_STATS_ADD( spriteSceneLayerMemoryUse, m_renderTarget.GetWidth() * m_renderTarget.GetHeight() * 4 );

	return true;
}

void Tr2Sprite2dLayer::ReleaseResources( TriStorage s )
{
	FreeRenderTarget();
}

void Tr2Sprite2dLayer::AllocateRenderTarget()
{
	if( m_renderTarget.IsValid() && ( (uint32_t)m_displayWidth == m_renderTarget.GetWidth() ) && ( (uint32_t)m_displayHeight == m_renderTarget.GetHeight() ) )
	{
		return;
	}

	m_isDirty = true;

	FreeRenderTarget();

	PrepareResources();
}

void Tr2Sprite2dLayer::FreeRenderTarget()
{
	CCP_STATS_ADD( spriteSceneLayerMemoryUse, -(double)( m_renderTarget.GetWidth() * m_renderTarget.GetHeight() * 4 ) );

	m_renderTarget = Tr2TextureAL();
}

unsigned int Tr2Sprite2dLayer::GetVertexCount()
{
	CCP_ASSERT_M( false, "Tr2Sprite2dLayer::GetVertexCount not implemented" );
	return 0;
}

void Tr2Sprite2dLayer::SetTextureSecondary( ITr2Sprite2dTexture* t )
{
	if( t != m_textureSecondary )
	{
		if( m_textureSecondary )
		{
			m_textureSecondary->UnregisterForChangeNotification( this );
		}
		m_textureSecondary = t;
		if( m_textureSecondary )
		{
			m_textureSecondary->RegisterForChangeNotification( this );
		}
		SetDirty();
	}
}

ITr2Sprite2dTexture* Tr2Sprite2dLayer::GetTextureSecondary() const
{
	return m_textureSecondary;
}

void Tr2Sprite2dLayer::Sprite2dTextureChanged( ITr2Sprite2dTexture* p )
{
	SetDirty();
}
