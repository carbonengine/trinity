// Copyright © 2011 CCP ehf.

#pragma once
#ifndef Tr2Sprite2dTextObject_h
#define Tr2Sprite2dTextObject_h

#include "Tr2SpriteObject.h"
#include "Tr2DeviceResource.h"
#include "Font/Tr2FontMeasurer.h"

BLUE_DECLARE( Tr2Sprite2dTextObject );
BLUE_DECLARE( Tr2FontMeasurer );
BLUE_DECLARE_INTERFACE( ITr2Sprite2dTexture );

class Tr2Sprite2dTextObject : public Tr2SpriteObject,
							  public Tr2DeviceResource,
							  public ITr2FontMeasurerNotifyTarget
{
public:
	EXPOSE_TO_BLUE();
	Tr2Sprite2dTextObject( IRoot* lockobj = nullptr );
	~Tr2Sprite2dTextObject();

	void SetFontMeasurer( Tr2FontMeasurer* m );
	Tr2FontMeasurer* GetFontMeasurer() const;

	bool IsAuxMouseover();

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

public:
	//////////////////////////////////////////////////////////////////////////
	// ITr2FontMeasurerNotifyTarget
	virtual void FontMeasurerChanged( Tr2FontMeasurer* p );

private:
	Tr2FontMeasurerPtr m_fontMeasurer;

	float m_pickRadius;
	float m_textWidth;
	float m_textHeight;

	bool m_useShadowSpriteEffect;
	Tr2SpriteObjectEffect m_shadowSpriteEffect;

	// hack
	ITr2Sprite2dTexturePtr m_texturePrimary;
	bool m_useSizeFromTexture;

	//denotes that this label has at least one auxiliary tooltip (as an inline object)
	bool m_hasAuxiliaryTooltip;
};

TYPEDEF_BLUECLASS( Tr2Sprite2dTextObject );

#endif