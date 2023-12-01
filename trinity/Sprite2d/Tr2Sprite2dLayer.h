#pragma once

#ifndef Tr2Sprite2dLayer_h
#define Tr2Sprite2dLayer_h


#include "Tr2Sprite2dContainer.h"
#include "Tr2DeviceResource.h"

BLUE_DECLARE( Tr2Sprite2dLayer );
BLUE_DECLARE_IVECTOR( ITr2SpriteObject );
BLUE_DECLARE( Tr2Effect );

class Tr2Sprite2dLayer:
	public Tr2Sprite2dContainer,
	public Tr2DeviceResource,
	public ITr2Sprite2dTextureNotifyTarget
{
public:
	EXPOSE_TO_BLUE();
	Tr2Sprite2dLayer( IRoot* lockobj = NULL );
	~Tr2Sprite2dLayer();

	//////////////////////////////////////////////////////////////////////////
	// ITr2SpriteObject
	unsigned int GetVertexCount();
	void GatherSprites( Tr2Sprite2dScene* renderer );
	void SetChildDirty( ITr2SpriteObject* child );

	//////////////////////////////////////////////////////////////////////////
	// ITr2Sprite2dTextureNotifyTarget
	void Sprite2dTextureChanged( ITr2Sprite2dTexture* p );

	//////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();

	void AllocateRenderTarget();
	void FreeRenderTarget();

	void SetTextureSecondary( ITr2Sprite2dTexture* t );
	ITr2Sprite2dTexture* GetTextureSecondary() const;

private:
	Tr2TextureAL m_renderTarget;
	
	// If m_clearBackground is set, background is cleared to this color
	Color m_backgroundColor;

	Color m_color;

	// Does this layer need a call to Clear before rendering children?
	bool m_clearBackground;

	Tr2SpriteObjectBlendMode m_blendMode;
	Tr2SpriteObjectEffect m_spriteEffect;
	ITr2Sprite2dTexturePtr m_textureSecondary;
};

TYPEDEF_BLUECLASS( Tr2Sprite2dLayer );

#endif //Tr2Sprite2dLayer_h
