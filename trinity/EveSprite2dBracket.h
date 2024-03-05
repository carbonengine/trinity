////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2012
// Copyright:	CCP 2012
//

#pragma once
#ifndef EveSprite2dBracket_h
#define EveSprite2dBracket_h

BLUE_DECLARE( EveSprite2dBracket );
BLUE_DECLARE( Tr2AtlasTexture );

BLUE_CLASS( EveSprite2dBracket ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveSprite2dBracket( IRoot* lockobj = nullptr );

	Vector2 GetTranslation() const;
	void SetTranslation( Vector2 val );

	Tr2AtlasTexture* GetIcon() const;
	const Color& GetColor() const;
	
	void SetDisplay( bool b );
	bool IsDisplay() const;

private:
	Vector2 m_translation;
	Color m_color;
	Tr2AtlasTexturePtr m_icon;
	bool m_display;
};

TYPEDEF_BLUECLASS( EveSprite2dBracket );

#endif // EveSprite2dBracket_h