// Copyright © 2026 CCP ehf.

#pragma once

#include "ITr2RenderNode.h"

BLUE_DECLARE( Tr2Sprite2dScene );


BLUE_CLASS( Tr2RenderNodeSprite2dScene ) :
	public ITr2RenderNode
{
public:
	EXPOSE_TO_BLUE();

	bool Validate( const Span<const Tr2BitmapDimensions>& destDimensions, const Span<const BlueSharedString>& outputs, Be::Time realTime, Be::Time simTime ) override;
	void Execute( const Span<const Tr2TextureAL>& destinations, const Span<TempOutput>& outputs, Be::Time realTime, Be::Time simTime, const Tr2ProfileTimer& rootTimer, Tr2RenderContext& renderContext ) override;

private:
	Tr2Sprite2dScenePtr m_scene;
	ITr2RenderNodePtr m_background;
};
TYPEDEF_BLUECLASS( Tr2RenderNodeSprite2dScene );
