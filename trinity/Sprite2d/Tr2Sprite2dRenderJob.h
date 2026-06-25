// Copyright © 2011 CCP ehf.

#pragma once
#ifndef Tr2Sprite2dRenderJob_h
#define Tr2Sprite2dRenderJob_h

#include "ITr2SpriteObject.h"
#include "Tr2SpriteObject.h"

BLUE_DECLARE( TriRenderJob );
BLUE_DECLARE( Tr2Sprite2dRenderJob );

class Tr2Sprite2dRenderJob : public Tr2SpriteObjectBase
{
public:
	EXPOSE_TO_BLUE();

	Tr2Sprite2dRenderJob( IRoot* lockobj = NULL );
	~Tr2Sprite2dRenderJob();

	//////////////////////////////////////////////////////////////////////////
	// ITr2SpriteObject
	unsigned int GetVertexCount();
	virtual void GatherSprites( Tr2Sprite2dScene* renderer );
	virtual ITr2SpriteObject* PickPoint( float x, float y, Tr2Sprite2dScene* renderer );

private:
	TriRenderJobPtr m_renderJob;
};

TYPEDEF_BLUECLASS( Tr2Sprite2dRenderJob );

#endif
