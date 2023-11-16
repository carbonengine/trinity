////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2012
// Copyright:	CCP 2012
//

#pragma once
#ifndef EveSprite2dBracketRenderer_h
#define EveSprite2dBracketRenderer_h

#include "Sprite2d/Tr2SpriteObject.h"
#include "Tr2DeviceResource.h"

BLUE_DECLARE( EveSprite2dBracketRenderer );
BLUE_DECLARE( EveSprite2dBracket );
BLUE_DECLARE_VECTOR( EveSprite2dBracket );
BLUE_DECLARE( Tr2AtlasTexture );

class EveSprite2dBracketRenderer :
	public Tr2SpriteObject,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();
	EveSprite2dBracketRenderer( IRoot* lockobj = nullptr );

	//////////////////////////////////////////////////////////////////////////
	// ITr2SpriteObject
	unsigned int GetVertexCount();
	void GatherSprites( Tr2Sprite2dScene* renderer );
	ITr2SpriteObject* PickPoint( float x, float y, Tr2Sprite2dScene* renderer );

	//////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();

private:
	PEveSprite2dBracketVector m_brackets;
	Tr2AtlasTexturePtr m_iconAtlas;

	// The size of the vertex/index buffers in terms of brackets
	unsigned int m_bracketCountInBuffers;

	// Vertex/index buffers. They are recreated whenever the count of
	// brackets changes.
	Tr2BufferAL	m_vertexBuffer;
	Tr2BufferAL	m_indexBuffer;
};

TYPEDEF_BLUECLASS( EveSprite2dBracketRenderer );

#endif // EveSprite2dBracketRenderer_h