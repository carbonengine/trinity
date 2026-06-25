// Copyright © 2023 CCP ehf.

#pragma once

#ifndef Tr2SpriteObject_h
#define Tr2SpriteObject_h


#include "ITr2SpriteObject.h"
#include "ITr2Sprite2dRenderer.h"
#include "ITr2Sprite2dTexture.h"
class Tr2Sprite2dScene;

BLUE_DECLARE( Tr2SpriteObject );
BLUE_DECLARE( Tr2Sprite2dPickingMask );

// This is defined in Tr2SpriteObject_Blue.cpp
extern Be::VarChooser Tr2SpriteObjectPickStateChooser[];
extern Be::VarChooser Tr2SpriteObjectEffectChooser[];
extern Be::VarChooser Tr2SpriteObjectBlendModeChooser[];
extern Be::VarChooser Tr2SpriteTargetChooser[];

class Tr2SpriteObjectBase : public ITr2SpriteObject,
							public INotify
{
public:
	EXPOSE_TO_BLUE();

	Tr2SpriteObjectBase();
	~Tr2SpriteObjectBase();

	bool GetDisplay() const;
	void SetDisplay( bool val );

	float GetDisplayX() const;
	void SetDisplayX( float val );

	float GetDisplayY() const;
	void SetDisplayY( float val );

	float GetDisplayWidth() const;
	void SetDisplayWidth( float val );

	float GetDisplayHeight() const;
	void SetDisplayHeight( float val );

#if BLUE_WITH_PYTHON
	PyObject* GetAssociatedObject() const;
	void SetAssociatedObject( PyObject* obj );
#endif

	//////////////////////////////////////////////////////////////////////////
	// ITr2SpriteObject
	void SetParent( ITr2SpriteObject* parent );
	void SetDirty();
	void SetChildDirty( ITr2SpriteObject* child );
	bool IsAuxMouseover();

	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* value );

protected:
	bool m_isDirty;

	// Note that these are weak references, to avoid a circular reference
	ITr2SpriteObject* m_parent;
	ITr2SpriteObject* m_auxMouseover;

	Vector2 m_translation;
	float m_displayWidth;
	float m_displayHeight;

	std::wstring m_name;
	bool m_display;
	Tr2SpriteObjectPickState m_pickState;

	Tr2Sprite2dPickingMaskPtr m_pickingMask;

#if BLUE_WITH_PYTHON
	PyObject* m_associatedObject;
#endif
};

class Tr2SpriteObject : public Tr2SpriteObjectBase
{
public:
	EXPOSE_TO_BLUE();

	Tr2SpriteObject( IRoot* lockobj = NULL );

	Color GetColor() const;
	void SetColor( Color val );

protected:
	void SetRegularRenderState( Tr2Sprite2dScene* renderer );
	void SetGlowRenderState( Tr2Sprite2dScene* renderer );
	void SetShadowRenderState( Tr2Sprite2dScene* renderer );
	void SetOutlineRenderState( Tr2Sprite2dScene* renderer );
	bool ValidateAndSetTextures( Tr2Sprite2dScene* renderer );

protected:
	Tr2SpriteObjectBlendMode m_blendMode;
	Tr2SpriteObjectEffect m_spriteEffect;
	Tr2SpriteTarget m_target;
	float m_glowBrightness;
	Color m_color;
	float m_depth;
	Vector2 m_shadowOffset;
	Color m_shadowColor;
	float m_glowFactor;
	float m_glowExpand;
	Color m_glowColor;
	Color m_outlineColor;
	float m_outlineThreshold;
};

TYPEDEF_BLUECLASS( Tr2SpriteObject );

class Tr2TexturedSpriteObject : public Tr2SpriteObject,
								public ITr2Sprite2dTextureNotifyTarget
{
public:
	EXPOSE_TO_BLUE();

	Tr2TexturedSpriteObject( IRoot* lockobj = NULL );
	~Tr2TexturedSpriteObject();

	void SetTexturePrimary( ITr2Sprite2dTexture* t );
	ITr2Sprite2dTexture* GetTexturePrimary() const;

	void SetTextureSecondary( ITr2Sprite2dTexture* t );
	ITr2Sprite2dTexture* GetTextureSecondary() const;

	//////////////////////////////////////////////////////////////////////////
	// ITr2Sprite2dTextureNotifyTarget
	void Sprite2dTextureChanged( ITr2Sprite2dTexture* p );

protected:
	bool ValidateAndSetTextures( Tr2Sprite2dScene* renderer );
	void SetValidatedTextures( Tr2Sprite2dScene* renderer );

protected:
	ITr2Sprite2dTexturePtr m_texturePrimary;
	ITr2Sprite2dTexturePtr m_textureSecondary;
};

TYPEDEF_BLUECLASS( Tr2TexturedSpriteObject );

#endif