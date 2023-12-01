//////////////////////////////////////////////////////////////////////////
//
// Created: April 2012
// Copyright CCP 2012
//

#pragma once

#ifndef Tr2Sprite2dArc_h
#define Tr2Sprite2dArc_h

#include "Tr2SpriteObject.h"

BLUE_DECLARE( Tr2Sprite2dArc );
BLUE_DECLARE_INTERFACE( ITr2Sprite2dTexture );

class Tr2Sprite2dArc :
	public Tr2SpriteObject
{
public:
	EXPOSE_TO_BLUE();
	Tr2Sprite2dArc( IRoot* lockobj = NULL );
	~Tr2Sprite2dArc();

	//////////////////////////////////////////////////////////////////////////
	// ITr2SpriteObject
	unsigned int GetVertexCount();
	void GatherSprites( Tr2Sprite2dScene* renderer );
	ITr2SpriteObject* PickPoint( float x, float y, Tr2Sprite2dScene* renderer );

protected:
	void ClearVertices();

private:
	ITr2Sprite2dTexturePtr m_texturePrimary;
	ITr2Sprite2dTexturePtr m_textureSecondary;

	Tr2Sprite2dD3DVertex* m_renderVertices;
	unsigned int m_renderVertexCapacity;
	unsigned int m_renderVertexCount;
	TrackableStdVector<unsigned short> m_renderIndices;

	float m_radius;
	float m_startAngle;
	float m_endAngle;
	float m_lineWidth;
	Color m_lineColor;
	bool m_fill;
};

TYPEDEF_BLUECLASS( Tr2Sprite2dArc );

#endif