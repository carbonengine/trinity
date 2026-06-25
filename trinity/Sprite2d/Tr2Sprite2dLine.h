// Copyright © 2010 CCP ehf.

#pragma once

#ifndef Tr2Sprite2dLine_h
#define Tr2Sprite2dLine_h

#include "Tr2SpriteObject.h"

BLUE_DECLARE( Tr2Sprite2dLine );
BLUE_DECLARE_INTERFACE( ITr2Sprite2dTexture );

class Tr2Sprite2dLine : public Tr2TexturedSpriteObject
{
public:
	EXPOSE_TO_BLUE();
	Tr2Sprite2dLine( IRoot* lockobj = NULL );

	//////////////////////////////////////////////////////////////////////////
	// ITr2SpriteObject
	unsigned int GetVertexCount();
	void GatherSprites( Tr2Sprite2dScene* renderer );
	ITr2SpriteObject* PickPoint( float x, float y, Tr2Sprite2dScene* renderer );

private:
	Tr2Sprite2dD3DVertex m_vertices[4];

	Vector2 m_translationFrom;
	Vector2 m_translationTo;
	float m_widthFrom;
	float m_widthTo;
	Color m_colorFrom;
	Color m_colorTo;
	float m_textureWidth;
	float m_textureOffsetBase;
	float m_textureOffset;
};

TYPEDEF_BLUECLASS( Tr2Sprite2dLine );

#endif