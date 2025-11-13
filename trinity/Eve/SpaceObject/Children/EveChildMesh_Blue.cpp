#include "StdAfx.h"
#include "EveChildMesh.h"

BLUE_DEFINE( EveChildMesh );

const Be::ClassInfo* EveChildMesh::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveChildMesh, "" )
        MAP_INTERFACE( EveChildMesh )
        MAP_INTERFACE( EveEntity )
		MAP_INTERFACE( IEveSpaceObjectDecalOwner )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( ITr2GrannyAnimationOwner )
		MAP_INTERFACE( IEveSpaceObjectAttachmentOwner )
		MAP_INTERFACE( ITr2LightOwner )
		MAP_INTERFACE( IEveShadowCaster )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST	)
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "castShadow", m_castShadow, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "updateAnimation", m_updateAnimation, "Should the object update its animation updater every frame", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "mesh", m_mesh, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"animationUpdater",
			m_animationUpdater,
			"Granny animation exposure",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "lowestLodVisible", m_lowestLodVisible, "Lowest LOD this guy is visible\n:jessica-group: LOD", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"minScreenSize", 
			m_minScreenSize, 
			"Minimal size of object on screen, objects smaller than this size are not rendered.\n:jessica-group: LOD", 
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "currentScreenSize", m_currentScreenSize, "Screen size for last frame\n:jessica-group: LOD", Be::READ)
		MAP_ATTRIBUTE( "currentInstanceScreenSize", m_currentInstanceScreenSize, "Screen size of instances for last frame\n:jessica-group: LOD", Be::READ )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling,"", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "localTransform", m_localTransform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "worldTransform", m_worldTransform, "", Be::READ )
		MAP_ATTRIBUTE( "useSRT", m_useSRT, "Should local transform be built from scaling, rotation and translation attributes.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "staticTransform", m_staticTransform, "Does local transform need to be rebuilt every frame.", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "sortValueScale", m_sortValueScale, "Scale for camera distance used for transparent sort", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sortValueOffset", m_sortValueOffset, "Offset for camera distance used for transparent sort", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "transformModifiers", m_transformModifiers, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "origin", m_origin, "Where did this effect originate from", Be::READ )
		MAP_ATTRIBUTE( "decals", m_decals, "Decals", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "attachments", m_attachments, "Item sets attached to the object", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "lights", m_lights, "Lights attached to the object", Be::READ | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "RebuildLocalTransform", RebuildLocalTransform, "Rebuilds local transform if useSRT is set." )
		
		MAP_ATTRIBUTE_WITH_CHOOSER( "reflectionMode", m_reflectionMode, "When is this object rendered into the cubemap", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, EntityComponents::ReflectionModeChooser );
		
		MAP_METHOD_AND_WRAP(
			"GetMorphTargetNames",
			GetMorphTargetNames,
			"GetMorphTargetNames()\n\n"
			"Returns names of the morph targets of the mesh. Empty if no mesh has been set.\n" )
		MAP_METHOD_AND_WRAP(
			"SetMorphTargetWeight",
			SetMorphTargetWeight,
			"SetMorphTargetWeight( name, weight )\n\n"
			"Sets the weight of the morph target. If no morph target with that name could be found, then the call is ignored.\n"
			":param name: morph target name\n"
			":param weight: morph target weight\n" )
		MAP_METHOD_AND_WRAP(
			"GetMorphTargetWeight",
			GetMorphTargetWeight,
			"GetMorphTargetWeight( name )\n\n"
			"Returns the weight of the morph target. Returns 0 if no morph target with that name was found.\n"
			":param name: morph target name\n" )

    EXPOSURE_END()
}