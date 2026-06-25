// Copyright © 2023 CCP ehf.

#pragma once
#ifndef _TRISTEPRENDERTEXTURE_H_
#define _TRISTEPRENDERTEXTURE_H_


#include "TriRenderStep.h"
#include "ITr2TextureProvider.h"

BLUE_DECLARE( Tr2AtlasTexture );
BLUE_DECLARE( Tr2RenderTarget );
BLUE_DECLARE( Tr2DepthStencil );
BLUE_DECLARE( TriTextureRes );

BLUE_CLASS( TriStepRenderTexture ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepRenderTexture( IRoot* lockobj = 0 );
	~TriStepRenderTexture( void );

	//RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

	void SetTexture( ITr2TextureProvider * tex );
	void SetTexture( Tr2AtlasTexture * tex );

private:
	void BlankOut();
	TriStepResult ClearIfFail( bool result, Tr2RenderContext& renderContext );
	ITr2TextureProviderPtr m_texture;
	Tr2AtlasTexturePtr m_atlasTexture;
	Vector2 m_tlTexCoord;
	Vector2 m_brTexCoord;
	Vector2 m_textureSize;
	unsigned m_failClearColor;
};

TYPEDEF_BLUECLASS( TriStepRenderTexture );

#endif