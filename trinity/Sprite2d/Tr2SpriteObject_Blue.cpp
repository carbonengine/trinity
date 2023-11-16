#include "StdAfx.h"
#include "Tr2SpriteObject.h"

BLUE_DEFINE_ABSTRACT( Tr2SpriteObjectBase );
BLUE_DEFINE_ABSTRACT( Tr2SpriteObject );
BLUE_DEFINE_ABSTRACT( Tr2TexturedSpriteObject );

Be::VarChooser Tr2SpriteObjectPickStateChooser[] =
{
	{ "TR2_SPS_OFF",		BeCast( TR2_SPS_OFF ),			"Picking is disabled" },
	{ "TR2_SPS_ON",			BeCast( TR2_SPS_ON ),			"Picking is enabled" },
	{ "TR2_SPS_CHILDREN",	BeCast( TR2_SPS_CHILDREN ),		"Only children are pickable" },
	{ 0 }
};

Be::VarChooser Tr2SpriteObjectBlendModeChooser[] =
{
	{ "TR2_SBM_NONE",		BeCast( TR2_SBM_NONE ),			"No blending" },
	{ "TR2_SBM_BLEND",		BeCast( TR2_SBM_BLEND ),		"Regular alpha blending" },
	{ "TR2_SBM_ADD",		BeCast( TR2_SBM_ADD ),			"Additive blending" },
	{ "TR2_SBM_ADDX2",		BeCast( TR2_SBM_ADDX2 ),		"Additive blending, x2 brightness" },
	{ 0 }
};

Be::VarChooser Tr2SpriteTargetChooser[] = {
	{ "COLOR", BeCast( int( Tr2SpriteTarget::COLOR ) ), "Render to color target" },
	{ "GLOW", BeCast( int( Tr2SpriteTarget::GLOW ) ), "Render to glow target" },
	{ "COLOR_AND_GLOW", BeCast( int( Tr2SpriteTarget::COLOR_AND_GLOW ) ), "Render both to color and glow targets" },
	{ 0 }
};

Be::VarChooser Tr2SpriteObjectEffectChooser[] =
{
	{ "TR2_SFX_FILL",			BeCast( TR2_SFX_FILL ),			"No texture - simple fill" },
	{ "TR2_SFX_FILL_AA",		BeCast( TR2_SFX_FILL_AA ),		"No texture - simple fill, with anti-aliasing" },
	{ "TR2_SFX_COPY",			BeCast( TR2_SFX_COPY ),			"Primary texture copied" },
	{ "TR2_SFX_FONT",			BeCast( TR2_SFX_FONT ),			"Font rendered by FreeType" },
	{ "TR2_SFX_DOT",			BeCast( TR2_SFX_DOT ),			"Dot product of primary texture and direction vector" },
	{ "TR2_SFX_DROPSHADOW",		BeCast( TR2_SFX_DROPSHADOW ),	"Primary texture copied, plus a drop shadow" },
	{ "TR2_SFX_MODULATE",		BeCast( TR2_SFX_MODULATE ),		"Primary and secondary textures modulated (multiplied)" },
	{ "TR2_SFX_MASK",			BeCast( TR2_SFX_MASK ),			"Alpha channel of secondary used as mask for primary" },
	{ "TR2_SFX_NOALPHA",		BeCast( TR2_SFX_NOALPHA ),		"Primary texture copied, alpha forced to 1.0" },
	{ "TR2_SFX_COLOROVERLAY",	BeCast( TR2_SFX_COLOROVERLAY ),	"Primary texture colorized with an overlay effect" },
	{ "TR2_SFX_SOFTLIGHT",		BeCast( TR2_SFX_SOFTLIGHT ),	"Primary texture colorized with a softlight effect" },
	{ "TR2_SFX_BLUR",			BeCast( TR2_SFX_BLUR ),			"Primary texture is blurred" },
	{ "TR2_SFX_BLURBACKGROUNDCOLORED",	BeCast( TR2_SFX_BLURBACKGROUNDCOLORED ),	"Primary texture is blurred, uvs are calculated from screen coords" },
	{ "TR2_SFX_BLURBACKGROUND",	BeCast( TR2_SFX_BLURBACKGROUND ),	"Primary texture is blurred, uvs are calculated from screen coords" },
	{ "TR2_SFX_GLOW",			BeCast( TR2_SFX_GLOW ),			"Primary texture is blurred, used for glow effects" },

	{ "TR2_SFX_NONE",		BeCast( TR2_SFX_NONE ),			"Nothing is rendered" },
	{ 0 }
};


BLUE_REGISTER_ENUM( "Tr2SpriteObjectBlendMode", Tr2SpriteObjectBlendMode, Tr2SpriteObjectBlendModeChooser );
BLUE_REGISTER_ENUM( "Tr2SpriteObjectEffect", Tr2SpriteObjectEffect, Tr2SpriteObjectEffectChooser );
BLUE_REGISTER_ENUM_EX( "Tr2SpriteTarget", Tr2SpriteTarget, Tr2SpriteTargetChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::ClassInfo* Tr2SpriteObjectBase::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2SpriteObjectBase, "" )
		MAP_INTERFACE( ITr2SpriteObject )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE
		(
			"name", 
			m_name, 
			"Name of this sprite", 
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"auxMouseover", 
			m_auxMouseover, 
			"The auxiliary mouseover object", 
			Be::READ
		)

		MAP_PROPERTY
		(
			"display",
			GetDisplay, SetDisplay,
			"If true, object is displayed. If false, the object is not displayed."
		)

		MAP_ATTRIBUTE
		(
			"isDirty",
			m_isDirty,
			"Has something changed since last render?",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"displayX", 
			m_translation.x,
			"X-coordinate of this sprite",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"displayY", 
			m_translation.y,
			"Y-coordinate of this sprite",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"displayWidth", 
			m_displayWidth,
			"Width of this sprite",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"displayHeight", 
			m_displayHeight,
			"Height of this sprite",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE_WITH_CHOOSER
		(
			"pickState",
			m_pickState,
			"This state controls how the object responds to picking",
			Be::READWRITE | Be::PERSIST | Be::ENUM,
			Tr2SpriteObjectPickStateChooser
		)

		MAP_ATTRIBUTE(
			"pickingMask",
			m_pickingMask,
			"Mouse picking mask",
			Be::READWRITE
		)

		MAP_METHOD_AND_WRAP
		(
			"SetDirty",
			SetDirty,
			"Marks this object as dirty and informs its parent"
		)

#if BLUE_WITH_PYTHON
		MAP_PROPERTY
		(
			"associatedObject",
			GetAssociatedObject, SetAssociatedObject,
			"Weak reference to a Python object associated with this sprite object."
		)
#endif

	EXPOSURE_END()
}

const Be::ClassInfo* Tr2SpriteObject::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2SpriteObject, "" )

		MAP_ATTRIBUTE_WITH_CHOOSER
		(
			"blendMode",
			m_blendMode,
			"Blendmode controls how the object blends with the background",
			Be::READWRITE | Be::ENUM | Be::NOTIFY,
			Tr2SpriteObjectBlendModeChooser
		)

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"spriteTarget",
			m_target,
			"Render target for the object: either color or glow target",
			Be::READWRITE | Be::ENUM | Be::NOTIFY,
			Tr2SpriteTargetChooser )

		MAP_ATTRIBUTE(
			"glowBrightness",
			m_glowBrightness,
			"Full-screen glow effect brightness factor for this sprite object. See spriteTarget.",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE_WITH_CHOOSER
		(
			"spriteEffect",
			m_spriteEffect,
			"Sprite effect controls how textures are used to render the object",
			Be::READWRITE | Be::ENUM | Be::NOTIFY,
			Tr2SpriteObjectEffectChooser
		)

		MAP_ATTRIBUTE
		(
			"color", 
			m_color,
			"Color of the sprite object",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"depth",
			m_depth,
			"Depth value to use when rendered in 3d - relative to parent.",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"shadowOffset",
			m_shadowOffset,
			"Optional shadow offset. If this is non-zero, a second copy of this\n"
			"object is rendered with 'shadowColor' before the regular rendering.\n\n"
			":jessica-group: Shadow",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"shadowColor",
			m_shadowColor,
			"Shadow color - see 'shadowOffset'\n"
			":jessica-group: Shadow",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"glowFactor",
			m_glowFactor,
			"Optional glow factor - if this is non-zero, a second copy of this\n"
			"object is rendered, blurred with glowFactor, after the regular rendering.\n"
			"'glowExpand' determines if this copy is expanded as well.",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"glowExpand",
			m_glowExpand,
			"If non-zero, a second copy of this object is rendered after the regular\n"
			"rendering, expanded by this factor. 'glowFactor' determines if this copy\n"
			"is blurred as well.",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"glowColor",
			m_glowColor,
			"Glow color - see 'shadowOffset'",
			Be::READWRITE | Be::NOTIFY
		)

	EXPOSURE_CHAINTO( Tr2SpriteObjectBase )
}

const Be::ClassInfo* Tr2TexturedSpriteObject::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2TexturedSpriteObject, "" )
		MAP_PROPERTY
		(
			"texturePrimary",
			GetTexturePrimary, SetTexturePrimary,
			"Primary texture for this sprite"
		)

		MAP_PROPERTY
		(
			"textureSecondary",
			GetTextureSecondary, SetTextureSecondary,
			"Secondary texture for this sprite"
		)

	EXPOSURE_CHAINTO( Tr2SpriteObject )
}
