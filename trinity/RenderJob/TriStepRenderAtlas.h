// Copyright © 2023 CCP ehf.

#pragma once
#ifndef TriStepRenderAtlas_h
#define TriStepRenderAtlas_h


#include "TriRenderStep.h"

class Tr2TextureAtlas;
class Tr2AtlasTexture;
class Tr2Effect;
class TriVariable;

BLUE_CLASS( TriStepRenderAtlas ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepRenderAtlas( IRoot* lockobj = 0 );
	~TriStepRenderAtlas();

	void py__init__( Tr2TextureAtlas * atlas, Tr2AtlasTexture * texture );

	//RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

	// Python __init__ constructor
	void SetAtlas( Tr2TextureAtlas * atlas );
	void SetFocus( Tr2AtlasTexture * texture );

private:
	//TODO FIXME what's up with none of these being smart ptrs?
	Tr2Effect* m_areaEffect;
	TriVariable* m_simpleOutColourHandle;
	Tr2TextureAtlas* m_atlas;
	Tr2AtlasTexture* m_focus;
	//really should steal these from a TriStepRenderTexture
	Vector2 m_tlTexCoord;
	Vector2 m_brTexCoord;
	bool m_showFree;
	bool m_showUsed;
	Vector4 m_borderColour, m_focusColour, m_freeColour;
};

TYPEDEF_BLUECLASS( TriStepRenderAtlas );

#endif //TriStepRenderAtlas_h
