// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2RenderNodeSprite2dScene.h"
#include "Sprite2d/Tr2Sprite2dScene.h"


BLUE_DEFINE( Tr2RenderNodeSprite2dScene );

const Be::ClassInfo* Tr2RenderNodeSprite2dScene::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2RenderNodeSprite2dScene, "" )
		MAP_INTERFACE( ITr2RenderNode )

		MAP_ATTRIBUTE(
			"background",
			m_background,
			"Node to render at the background of the 2D scene. Can be None.",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"scene",
			m_scene,
			"2D scene to render",
			Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}
