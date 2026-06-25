// Copyright © 2011 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dTextObject.h"
#include "Font/Tr2FontMeasurer.h"
#include "Tr2Sprite2dScene.h"

Tr2Sprite2dTextObject::Tr2Sprite2dTextObject( IRoot* lockobj /*= nullptr */ ) :
	m_textWidth( 0.0f ),
	m_textHeight( 0.0f ),
	m_pickRadius( 0.0f ),
	m_shadowSpriteEffect( TR2_SFX_FONT ),
	m_useShadowSpriteEffect( false ),
	m_hasAuxiliaryTooltip( false ),
	m_useSizeFromTexture( false )
{
	m_spriteEffect = TR2_SFX_FONT;
}

Tr2Sprite2dTextObject::~Tr2Sprite2dTextObject()
{
	if( m_fontMeasurer )
	{
		m_fontMeasurer->UnregisterForChangeNotification( this );
	}
}

void Tr2Sprite2dTextObject::GatherSprites( Tr2Sprite2dScene* renderer )
{
	if( !m_display || ( m_textWidth <= 0.0f ) || ( m_textHeight <= 0.0f ) )
	{
		return;
	}

	if( m_fontMeasurer )
	{
		auto dropShadow = m_shadowOffset.x != 0 || m_shadowOffset.y != 0;
		if( m_isDirty )
		{
			m_fontMeasurer->PrepareSprites(
				renderer,
				m_translation,
				m_color,
				m_spriteEffect,
				m_blendMode,
				m_target,
				m_glowBrightness,
				dropShadow,
				m_shadowOffset,
				m_shadowColor,
				m_useShadowSpriteEffect ? m_shadowSpriteEffect : m_spriteEffect );
			m_isDirty = false;
		}

		auto left = m_translation.x + std::min( m_shadowOffset.x, 0.f );
		auto top = m_translation.y + std::min( m_shadowOffset.y, 0.f );
		auto width = m_textWidth + abs( m_shadowOffset.x );
		auto height = m_textHeight + abs( m_shadowOffset.x );
		if( ( m_spriteEffect == TR2_SFX_BLUR ) || ( m_spriteEffect == TR2_SFX_GLOW ) )
		{
			renderer->PushClipRectangle( m_translation.x - 1 + std::min( m_shadowOffset.x, 0.f ), m_translation.y - 1 + std::min( m_shadowOffset.y, 0.f ), m_textWidth + 2 + abs( m_shadowOffset.x ), m_textHeight + 2 + abs( m_shadowOffset.y ) );
		}
		else
		{
			renderer->PushClipRectangle( m_translation.x + std::min( m_shadowOffset.x, 0.f ), m_translation.y + std::min( m_shadowOffset.y, 0.f ), m_textWidth + abs( m_shadowOffset.x ), m_textHeight + abs( m_shadowOffset.y ) );
		}
		m_fontMeasurer->SubmitSprites( renderer );
		renderer->PopClipRectangle();
	}
}

ITr2SpriteObject* Tr2Sprite2dTextObject::PickPoint( float x, float y, Tr2Sprite2dScene* renderer )
{
	if( !m_display )
	{
		return nullptr;
	}

	if( m_pickState == TR2_SPS_ON || m_hasAuxiliaryTooltip )
	{
		if( renderer->IsInside( Vector2( x, y ), m_translation, m_displayWidth, m_displayHeight, m_pickRadius ) )
		{
			return this;
		}
	}

	return nullptr;
}

unsigned int Tr2Sprite2dTextObject::GetVertexCount()
{
	if( !m_display || !m_fontMeasurer )
	{
		return 0;
	}
	return m_fontMeasurer->GetVertexCount();
}

void Tr2Sprite2dTextObject::ReleaseResources( TriStorage s )
{
	SetDirty();
	if( m_fontMeasurer )
	{
		m_fontMeasurer->ClearSprites();
	}
}

bool Tr2Sprite2dTextObject::OnPrepareResources()
{
	return true;
}

void Tr2Sprite2dTextObject::SetFontMeasurer( Tr2FontMeasurer* m )
{
	if( m != m_fontMeasurer )
	{
		if( m_fontMeasurer )
		{
			m_fontMeasurer->UnregisterForChangeNotification( this );
		}

		m_fontMeasurer = m;

		if( m_fontMeasurer )
		{
			m_fontMeasurer->RegisterForChangeNotification( this );
		}

		SetDirty();
	}
}

bool Tr2Sprite2dTextObject::IsAuxMouseover()
{
	// Return true if the label is not pickable and is a localization tooltip
	return ( m_pickState != TR2_SPS_ON && m_hasAuxiliaryTooltip );
}


Tr2FontMeasurer* Tr2Sprite2dTextObject::GetFontMeasurer() const
{
	return m_fontMeasurer;
}

void Tr2Sprite2dTextObject::FontMeasurerChanged( Tr2FontMeasurer* p )
{
	SetDirty();
}
