////////////////////////////////////////////////////////////
//
//    Created:   February 2023
//    Copyright: CCP 2023
//

#include "StdAfx.h"
#include "EveChildCloud2.h"
#include "EveCloudEditableVolume.h"

BLUE_DEFINE_INTERFACE( ITr2VolumetricRenderable );
BLUE_DEFINE( EveChildCloud2 );

const Be::ClassInfo* EveChildCloud2::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveChildCloud2, "Cloud space object child" )
        MAP_INTERFACE( EveChildCloud2 )
		MAP_INTERFACE( ITr2VolumetricRenderable )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IListNotify )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( ITr2LightOwner )
		MAP_INTERFACE( EveEntity )

		MAP_ATTRIBUTE( "name", m_name, "The name of the cloud", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "Toggle display", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE_WITH_CHOOSER( "minVisibleQuality", m_minVisibleQuality, "Minimal quality setting when we render the cloud", Be::READWRITE | Be::PERSIST | Be::ENUM, Tr2VolumetricQuality_Chooser )

		MAP_ATTRIBUTE( "castShadows", m_castShadows, "The cloud casts shadow onto opaque geometry if supported by the shader and scene settings", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "receiveShadows", m_receiveShadows, "The cloud receives shadows from opaque geometry if supported by the shader and scene settings", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sortingModifier", m_sortingModifier, "Affects the transparency sorting", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "effect", m_effect, "Shader used for the rendering", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "reflectionEffect", m_reflectionEffect, "Shader used for the rendering the cloud into the reflection probe", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "lights", m_lights, "List of dynamic lights", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "lightmap", m_lightMap, "Lightmap used for shadows/occlusion", Be::READ )
		MAP_PROPERTY( "lightmapDirty", IsLightmapDirty, MarkLightmapDirty, "If the lightmap needs to be recalculated" )
		MAP_ATTRIBUTE( "lightmapSizeScale", m_lightmapSizeScale, "Lightmap used for shadows/occlusion", Be::READ )

		MAP_ATTRIBUTE( "scaling", m_scaling, "Object scaling", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "translation", m_translation, "Object local translation", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "Object local rotation", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "noiseTextureSize", m_noiseTextureSize, "Size (width) of the noise texture used by the cloud effect", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE_WITH_CHOOSER( "reflectionMode", m_reflectionMode, "When is this object rendered into the cubemap", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, EntityComponents::ReflectionModeChooser );

		MAP_ATTRIBUTE( "animation", m_animation, "Cloud texture animation", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"shadowMapDS",
			m_shadowMapDS,
			"Depth stencil used for shadows.",
			Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( 
			"minScreenSize", 
			m_minScreenSize, 
			"Minimal size of object on screen, objects smaller than this size are not rendered.\n:jessica-group: LOD", 
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "textureTiling", m_mapTiling[0], "Tiling for the main density/temerature texture. Used for camera-attached clouds.\n:jessica-group: Tiling", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "detailTiling1", m_mapTiling[1], "Tiling for detail texture 0. Used for camera-attached clouds.\n:jessica-group: Tiling", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "detailTiling2", m_mapTiling[2], "Tiling for detail texture 1. Used for camera-attached clouds.\n:jessica-group: Tiling", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "mapOffset0", m_mapOffsets[0], "Texture offset 0. Used for debugging camera-attached clouds.\n:jessica-group: Tiling", Be::READ )
		MAP_ATTRIBUTE( "mapOffset1", m_mapOffsets[1], "Texture offset 1. Used for debugging camera-attached clouds.\n:jessica-group: Tiling", Be::READ )
		MAP_ATTRIBUTE( "mapOffset2", m_mapOffsets[2], "Texture offset 2. Used for debugging camera-attached clouds.\n:jessica-group: Tiling", Be::READ )

	EXPOSURE_END()
}
