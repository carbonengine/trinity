#include "StdAfx.h"
#include "Tr2Transform.h"

BLUE_DEFINE_ABSTRACT( Tr2Transform );

Be::VarChooser ModifierChooser[] =
{
	{
		"None",     
		BeCast( TR2TM_NONE ),     
		"No modifier on transform"
	},
	{
		"Billboard",     
		BeCast( TR2TM_BILLBOARD ),     
		"Aligned with the camera - useful for sprites"
	},
	{
		"Translate with camera",     
		BeCast( TR2TM_TRANSLATE_WITH_CAMERA ),     
		"Translates with the camera, rotation is not affected"
	},
	{
		"Look at camera",     
		BeCast( TR2TM_LOOK_AT_CAMERA ),     
		"Aligned with the camera - similar to Billboard, but works for 3D objects"
	},
	{
		"Simple halo",     
		BeCast( TR2TM_SIMPLE_HALO ),     
		"Aligned with the camera, scales with angle to camera - useful for light glows"
	},
	{
		"Pre-translate with camera",
		BeCast( TR2TM_PRE_TRANSLATE_WITH_CAMERA ),
		"Translates with the camera, preserving local transform"
	},
	{
		"EVE Camera rotation aligned",     
		BeCast( TR2TM_EVE_CAMERA_ROTATION_ALIGNED ),     
		"Do not use in new content! Similar to billboard - used for converted content."
	},
	{
		"Booster",     
		BeCast( TR2TM_EVE_BOOSTER ),     
		"Do not use in new content! Something old..."
	},
	{
		"EVE Simple halo",     
		BeCast( TR2TM_EVE_SIMPLE_HALO ),     
		"Do not use in new content! Similar to simple halo - used for converted content."
	},
	{
		"EVE Camera rotation",
		BeCast( TR2TM_EVE_CAMERA_ROTATION ),     
		"Do not use in new content! Similar to billboard - used for converted content."
	},
	{ 0 }
};


const Be::ClassInfo* Tr2Transform::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2Transform,"" )
        MAP_INTERFACE( Tr2Transform )
		MAP_INTERFACE( ITr2Renderable )

		MAP_ATTRIBUTE_WITH_CHOOSER( "modifier", 
									m_modifier, 
									"Modifier on the transform for special effects like camera facing", 
									Be::READWRITE | Be::PERSIST | Be::ENUM, 
									ModifierChooser
								  )
		MAP_ATTRIBUTE( "display", m_display,
					   "If set, this transform hierarchy is displayed.\n"
					   "Note that turning off display does not automatically turn\n"
					   "off update.",
					   Be::READWRITE | Be::PERSIST
					 )
		MAP_ATTRIBUTE( "sortValueMultiplier",
					   m_sortValueMultiplier,
					   "Multiplier on the camera distance returned as the sort value. This can be used to affect\n"
					   "the sorting of individual objects in the same transform hierarchy.",
					   Be::READWRITE | Be::PERSIST
					 )
		MAP_ATTRIBUTE( "useDistanceBasedScale", 
					   m_useDistanceBasedScale, 
					   "If set, additional scaling is done based on distance from camera.", 
					   Be::READWRITE | Be::PERSIST 
					 )
		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "update", m_update, "If set, this transform hierarchy is updated every frame.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "localTransform", m_localTransform, "", Be::READ )
		MAP_ATTRIBUTE( "worldTransform", m_worldTransform, "", Be::READ )
		MAP_ATTRIBUTE( "distanceBasedScaleArg1", m_distanceBasedScaleArg1, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "distanceBasedScaleArg2", m_distanceBasedScaleArg2, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "mesh", m_mesh, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "curveSets", m_curveSets, "Curvesets for animating things", Be::READ | Be::PERSIST )

    EXPOSURE_END()
}
