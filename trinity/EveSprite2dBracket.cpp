// Copyright © 2012 CCP ehf.

#include "StdAfx.h"
#include "EveSprite2dBracket.h"

EveSprite2dBracket::EveSprite2dBracket( IRoot* lockobj /*= nullptr */ ) :
	m_translation( 0.0f, 0.0f ),
	m_color( 1.0, 1.0f, 1.0f, 1.0f ),
	m_display( true )
{
}

Vector2 EveSprite2dBracket::GetTranslation() const
{
	return m_translation;
}

void EveSprite2dBracket::SetTranslation( Vector2 val )
{
	m_translation = val;
}

Tr2AtlasTexture* EveSprite2dBracket::GetIcon() const
{
	return m_icon;
}

const Color& EveSprite2dBracket::GetColor() const
{
	return m_color;
}

bool EveSprite2dBracket::IsDisplay() const
{
	return m_display;
}

void EveSprite2dBracket::SetDisplay( bool b )
{
	m_display = b;
}
