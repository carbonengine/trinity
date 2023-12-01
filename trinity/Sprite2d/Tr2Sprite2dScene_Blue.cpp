#include "StdAfx.h"
#include "Tr2Sprite2dScene.h"
#include "TriProjection.h"
#include "TriView.h"
#include "TriViewport.h"

BLUE_DEFINE_INTERFACE( ITr2SpriteObject );
BLUE_DEFINE( Tr2Sprite2dScene );

Be::VarChooser Tr2Sprite2dRenderEffectChooser[] =
{
	{
		"S2D_RFX_COPY",
		BeCast( S2D_RFX_COPY ),
		"Copy primary texture"
	},
	{
		"S2D_RFX_BLUR",
		BeCast( S2D_RFX_BLUR ),
		"Blurring"
	},
	{
		"S2D_RFX_MODULATE",
		BeCast( S2D_RFX_MODULATE ),
		"Multiply primary and secondary textures"
	},
	{ 0 }
};

BLUE_REGISTER_ENUM( "Tr2Sprite2dRenderEffect", Tr2Sprite2dRenderEffect, Tr2Sprite2dRenderEffectChooser );

const Be::ClassInfo* Tr2Sprite2dScene::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2Sprite2dScene, "" )
        MAP_INTERFACE( Tr2Sprite2dScene )
		MAP_INTERFACE( ITr2Scene )
		MAP_INTERFACE( ITr2Updateable )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE
		(
			"name",
			m_name,
			"The name of this scene.",
			Be::READWRITE | Be::PERSIST
		)
		
		MAP_ATTRIBUTE
		(
			"display",
			m_display,
			"If true, object is displayed. If false, the object is not displayed.",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"drawWireFrame",
			m_drawWireFrame,
			"If set, the scene is rendered in wire frame",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"ignoreClip",
			m_ignoreClip,
			"If set, clipping is ignored (helps with performance analysis",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"lastPickPos",
			m_lastPickPos,
			"Position of last pick",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"isFullscreen",
			m_isFullscreen,
			"Is the scene fullscreen? If so, displayWidth and displayHeight"
			"\nare set every frame to match the fullscreen viewport",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"is2dRender",
			m_is2dRender,
			"If set, the scene is rendered full screen in 2D",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"is2dPick",
			m_is2dPick,
			"If set, the scene is rendered full screen in 2D",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"translation",
			m_translation,
			"Translation used to render this scene in 3D space",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"displayX", 
			m_translation.x, 
			"X-coordinate of this object", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"displayY", 
			m_translation.y, 
			"Y-coordinate of this object", 
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"rotation",
			m_rotation,
			"Rotation used to render this scene in 3D space",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"scaling",
			m_scaling,
			"Scaling used to render this scene in 3D space",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"displayWidth",
			m_displayWidth,
			"",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"displayHeight",
			m_displayHeight,
			"",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"depthMin", 
			m_depthMin, 
			"Minimum depth for objects in the scene.",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"depthMax", 
			m_depthMax, 
			"Maximum depth for objects in the scene.",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE_WITH_CHOOSER
		(
			"pickState",
			m_pickState,
			"This state controls how the object responds to picking",
			Be::READWRITE | Be::PERSIST | Be::ENUM,
			Tr2SpriteObjectPickStateChooser
		)

		MAP_ATTRIBUTE
		(
			"clearBackground",
			m_clearBackground,
			"If set, render target is cleared to backgroundColor before rendering the scene",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"backgroundColor", 
			m_backgroundColor, 
			"Background color for this layer", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"children",
			m_children,
			"Sprites contained in this scene",
			Be::READ | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"background",
			m_background,
			"Background objects of this scene. Background objects don't get alignment,\n"
			"but always fill the extent of the scene. They are never considered for\n"
			"picking, either.",
			Be::READ | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"clearFinishedCurveSets",
			m_clearFinishedCurveSets,
			"If set, curve sets are cleared once finished",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"curveSets",
			m_curveSets,
			"Curve sets used in this scene",
			Be::READ | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"ubershader2d",
			m_uberShader2d,
			"",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"ubershader3d",
			m_uberShader3d,
			"",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"defaultTextureUpdates",
			m_defaultTextureUpdates,
			"Whether or not to flash the default texture, causing extra texture locks every frame",
			Be::READWRITE 
		)

		MAP_ATTRIBUTE
		(
			"maxItemsToRender",
			m_maxItemsToRender,
			"The maximum number of sprites (and other items) to render - this can be used for\n"
			"debugging the scene to see individual sprites being rendered.",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"maxDrawCallsToRender",
			m_maxDrawCallsToRender,
			"The maximum number of draw calls to render - this can be used for\n"
			"debugging the scene to see individual draw calls being rendered.",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"maxSpriteCount",
			m_maxSpriteCount,
			"Maximum number of sprites that are rendered in one draw call. This\n"
			"does not apply for cached containers.",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"captureIndexDataCapacity",
			m_captureIndexDataCapacity,
			"Capacity of buffer used for indices while capturing display lists.\n"
			"This buffer grows automatically as needed - this is the number of\n"
			"indices, not byte size.",
			Be::READ
		)

		MAP_PROPERTY
		(
			"useLinearColorSpace",
			IsUsingLinearColorSpace,
			SetUseLinearColorSpace,
			"For TESTING sRGB vs linear color math"
		)

		MAP_PROPERTY(
			"gammaCorrectText",
			IsGammaCorrectingText,
			SetGammaCorrectText,
			"For TESTING sRGB vs linear color math" 
		)
		

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"PickObject",
			PickObject,
			1,
			"Returns the sprite object at the given coordinates (or None)\n\n"
			":param x: x-coordinate in screen space\n"
			":param y: y-coordinate in screen space\n"
			":param proj: Projection transform\n"
			":param view: View transform\n"
			":param vp: Viewport\n"
			":param pickFilter: object types to pick (unused)\n"
		)

    EXPOSURE_END()
}
