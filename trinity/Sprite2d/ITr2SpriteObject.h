#pragma once

#ifndef ITr2SpriteObject_h
#define ITr2SpriteObject_h

class Tr2Sprite2dScene;

BLUE_INTERFACE( ITr2SpriteObject ) : public IRoot
{
	virtual void GatherSprites( Tr2Sprite2dScene* renderer ) = 0;
	virtual ITr2SpriteObject* PickPoint( float x, float y, Tr2Sprite2dScene* renderer ) = 0;
	virtual void SetParent( ITr2SpriteObject* parent ) = 0;
	virtual void SetDirty() = 0;
	virtual void SetChildDirty( ITr2SpriteObject* child ) = 0;
	virtual bool IsAuxMouseover() = 0;
};

#endif