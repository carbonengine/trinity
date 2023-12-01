//////////////////////////////////////////////////////////////////////////
//
// Created: December 2010
// Copyright CCP 2010
//

#pragma once

#ifndef ITr2Sprite2dTexture_h
#define ITr2Sprite2dTexture_h

struct ITr2Sprite2dRenderer;
class Tr2AtlasTexture;
class Tr2Sprite2dScene;

struct ITr2Sprite2dTexture;

class ITr2Sprite2dTextureNotifyTarget
{
public:
	virtual void Sprite2dTextureChanged( ITr2Sprite2dTexture* p ) = 0;
};

BLUE_INTERFACE( ITr2Sprite2dTexture ) : public IRoot
{
	virtual void Apply( Tr2Sprite2dScene* renderer, unsigned int ix ) = 0;

	virtual unsigned int GetWidth() const = 0;
	virtual unsigned int GetHeight() const = 0;

	virtual float GetSrcWidth() const = 0;
	virtual float GetSrcHeight() const = 0;

	virtual bool IsLoading() const = 0;
	virtual bool IsGood() const = 0;

	virtual void RegisterForChangeNotification( ITr2Sprite2dTextureNotifyTarget* p ) = 0;
	virtual void UnregisterForChangeNotification( ITr2Sprite2dTextureNotifyTarget* p ) = 0;
};

#endif
