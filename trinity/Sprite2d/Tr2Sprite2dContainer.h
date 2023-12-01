#pragma once

#ifndef Tr2Sprite2dContainer_h
#define Tr2Sprite2dContainer_h


#include "Tr2SpriteObject.h"

BLUE_DECLARE( Tr2Sprite2dContainer );
BLUE_DECLARE_IVECTOR( ITr2SpriteObject );

class Tr2Sprite2dContainerBase:
     public Tr2SpriteObjectBase,
	 public IListNotify
{
public:
	EXPOSE_TO_BLUE();
	Tr2Sprite2dContainerBase( IRoot* lockobj = NULL );
	~Tr2Sprite2dContainerBase();

	//////////////////////////////////////////////////////////////////////////
	// ITr2SpriteObject
	void SetChildDirty( ITr2SpriteObject* child );

	//////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified(
		long event,		// BLUELISTEVENT values
		ssize_t key,
		ssize_t key2,
		IRoot* value,
		const IList* theList
		);

protected:
	float m_opacity;

	PITr2SpriteObjectVector m_children;
	PITr2SpriteObjectVector m_background;

};

class Tr2Sprite2dContainer:
	public Tr2Sprite2dContainerBase
{
public:
    EXPOSE_TO_BLUE();
    Tr2Sprite2dContainer( IRoot* lockobj = NULL );
	~Tr2Sprite2dContainer();

	//////////////////////////////////////////////////////////////////////////
	// ITr2SpriteObject
	void GatherSprites( Tr2Sprite2dScene* renderer );
	ITr2SpriteObject* PickPoint( float x, float y, Tr2Sprite2dScene* renderer );
	void SetDirty();

protected:
	void GatherSpritesHelper( Tr2Sprite2dScene* renderer );

protected:
	float m_depthMin;
	float m_depthMax;

	// Used for absolute alignment - resets the transforms applied
	// by parent to identity.
	bool m_absoluteCoordinates;

	// If set, children are clipped to the bounds of this container.
	bool m_clip;

	float m_pickRadius;

	bool m_cacheContents;
	bool m_cacheContentsHint;
	uint8_t m_dirtyFrameCount;
	uint8_t m_cleanFrameCount;
	struct Tr2Sprite2dDisplayList* m_cachedDisplayList;
};

TYPEDEF_BLUECLASS( Tr2Sprite2dContainer );

#endif //Tr2Sprite2dContainer_h
