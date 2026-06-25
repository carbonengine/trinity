// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2Blitter_H
#define Tr2Blitter_H


#include "Tr2DeviceResource.h"
#include "Tr2Variable.h"

BLUE_DECLARE( Tr2Material );
BLUE_DECLARE( Tr2Shader );
BLUE_DECLARE( Tr2Effect );

class Tr2TextureAL;

class Tr2Blitter : public Tr2DeviceResource
{
public:
	Tr2Blitter();
	~Tr2Blitter();

	// filtering options for blitting
	enum Filtering
	{
		FILTER_POINT = 0,
		FILTER_LINEAR,
	};

	// 2d blits unfiltered (point sampling)
	bool DrawInCameraSpace( Tr2RenderContext& renderContext, Tr2Shader* shader, Tr2Material* material );

	bool Draw( Tr2RenderContext& renderContext, const Tr2TextureAL& texture, const Vector2& tlTexCoord = Vector2( 0.0f, 0.0f ), const Vector2& brTexCoord = Vector2( 1.0f, 1.0f ), Filtering filter = FILTER_POINT );
	bool Draw( Tr2RenderContext& renderContext, const Tr2TextureAL& texture, const Vector2& tlTexCoord, const Vector2& brTexCoord, const Vector2& tlVertexCoord, const Vector2& brVertexCoord );

	bool Draw( Tr2RenderContext& renderContext, Tr2Material* effect, const Tr2TextureAL& texture );
	bool Draw( Tr2RenderContext& renderContext, Tr2Material* effect, const Tr2TextureAL& texture, const Vector2& tlTexCoord, const Vector2& brTexCoord );
	bool Draw( Tr2RenderContext& renderContext, Tr2Material* effect, const Tr2TextureAL& texture, const Vector2& tlTexCoord, const Vector2& brTexCoord, const Vector2& tlVertexCoord, const Vector2& brVertexCoord );

	bool Draw( Tr2RenderContext& renderContext, Tr2Material* effect );
	bool Draw( Tr2RenderContext& renderContext, Tr2Material* effect, const Vector2& tlTexCoord, const Vector2& brTexCoord );
	bool Draw( Tr2RenderContext& renderContext, Tr2Material* effect, const Vector2& tlTexCoord, const Vector2& brTexCoord, const Vector2& tlVertexCoord, const Vector2& brVertexCoord );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );

private:
	bool OnPrepareResources();

private:
	bool DrawHelper(
		Tr2RenderContext& renderContext,
		Tr2Shader* shader,
		Tr2Material* material,
		const Tr2TextureAL* halTexture,
		bool isCameraSpace,
		const Vector2& tlTexCoord = Vector2( 0.0f, 0.0f ),
		const Vector2& brTexCoord = Vector2( 1.0f, 1.0f ),
		const Vector2& tlVertexCoord = Vector2( 0.0f, 0.0f ),
		const Vector2& brVertexCoord = Vector2( 1.0f, 1.0f ) );

	unsigned int m_screenVertexDecl;
	Tr2BufferAL m_vertexBuffer;

	// blit effects
	Tr2EffectPtr m_blitEffect;
	Tr2EffectPtr m_blitFilteredEffect;

	// params for blitting
	Tr2Variable m_blitSourceVar;
	Tr2Variable m_mipLevelVar;
};

#endif // Tr2Blitter_H
