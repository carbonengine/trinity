#include "StdAfx.h"
#include "EveSpaceObject2.h"
#include "TriConstants.h"
#include "Vector3.h"

BLUE_DEFINE_INTERFACE( IEveSpaceObject2 );
BLUE_DEFINE_INTERFACE( IEveShadowCaster );
BLUE_DEFINE_INTERFACE( IEveLightReceiver );
BLUE_DEFINE_ABSTRACT( EveSpaceObject2 );

const Be::ClassInfo* EveSpaceObject2::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSpaceObject2, "" )
        MAP_INTERFACE( EveSpaceObject2 )
		MAP_INTERFACE( IEveShadowCaster )
		MAP_INTERFACE( IEveLightReceiver )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITr2Pickable )
		MAP_INTERFACE( ITriTargetable )
		MAP_INTERFACE( IWorldPosition )
		MAP_INTERFACE( ITr2ShLightingReceiver )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE
		( 
			"name",    
			m_name,    
			"", 
			Be::READWRITE | Be::PERSIST 
		)

		MAP_ATTRIBUTE
		( 
			"display", 
			m_display, 
			"Specifies whether to render the object or not", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE( "displayChildren", m_displayChildren, "Specifies whether to render this object's children or not", Be::READWRITE )

		MAP_ATTRIBUTE
		( 
			"update", 
			m_update, 
			"Specifies whether to update the object or not", 
			Be::READWRITE | Be::PERSIST 
		)

		MAP_ATTRIBUTE
		(
			"enableShadow", 
			m_enableShadow, 
			"If set, shadow is enabled",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		( 
			"debugShowBoundingBox", 
			m_debugShowBoundingBox, 
			"If set, bounding box is shown  (if scene is showing debug info).", 
			Be::READWRITE | Be::PERSIST 
		)

		MAP_ATTRIBUTE
		( 
			"debugShowMeshAreaBoundingBox",
			m_debugShowMeshAreaBoundingBox, 
			"If set, bounding boxes of the mesh areas are shown  (if scene is showing debug info).", 
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		( 
			"debugRenderDebugInfoForChildren", 
			m_debugRenderDebugInfoForChildren, 
			"If set, children are given a chance to render debug info (if scene is showing debug info).", 
			Be::READWRITE | Be::PERSIST 
		)

		MAP_ATTRIBUTE
		( 
			"debugShowDynamicBounds", 
			m_debugShowDynamicBounds, 
			"If set animation based bounding info is drawn(if scene is showing debug info).", 
			Be::READWRITE | Be::PERSIST 
		)

		MAP_ATTRIBUTE
		(
			"mesh",  
			m_mesh, 
			"Mesh for rendering space object", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"meshLod",  
			m_meshLod, 
			"Mesh with levels-of-detail for rendering space object", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"shadowEffect",  
			m_shadowEffect, 
			"Effect used to render into shadow map. If not set, object can't cast shadows.", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		( 
			"damageLocators",    
			m_persistedDamageLocators,
			"Blue structure list of EveDamageLocators (Vector3 and a Quaternion) that are read\n"
			"into the class and transformed with the ship frame-by-frame to give new turrets\n"
			"locations to shoot at on it.", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(    
			"translationCurve",
			m_ballPosition,
			"Vector function slot for attaching a destiny ball to set the position of a ship",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(    
			"rotationCurve",
			m_ballRotation,
			"Quaternion function slot for attaching a destiny ball to set the rotation of a ship",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(    
			"modelRotationCurve",
			m_modelRotation,
			"Used to add rotations to the basic rotation curve",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(    
			"modelTranslationCurve",
			m_modelTranslation,
			"Used to add animated translations to ships",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		( 
			"worldPosition", 
			m_worldPosition, 
			"Position in world space", 
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"modelScale", 
			m_modelScale, 
			"Scaling of this object (ONLY USED BY ASTEROIDS!)",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"spriteSets",
			m_spriteSets,
			"Sprite sets attached to the object",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"spotlightSets",
			m_spotlightSets,
			"Spotlight sets attached to the object",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"planeSets",
			m_planeSets,
			"Plane sets attached to the object",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		( 
			"locators", 
			m_locators, 
			"Locators for things such as turrets, boosters, etc. Locators can also come from a skeleton.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		( 
			"observers", 
			m_observers, 
			"Observers for pushing data between modules every frame. Currently used to push locator data out to the audio2 module.",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE( "decals", m_decals, "list of all decals on this space object.", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "particleEmittersGPU", m_particleEmittersGPU, "A list of GPU emitters owned by this object", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE
		(
			"estimatedPixelDiameter",
			m_estimatedPixelDiameter,
			"Estimated pixel diameter given the current view/projection transforms.\n",
			Be::READ
		)
		MAP_ATTRIBUTE
		(
			"estimatedPixelDiameterWithChildren",
			m_estimatedPixelDiameterWithChildren,
			"Estimated pixel diameter given the current view/projection transforms.\n",
			Be::READ
		)

		MAP_ATTRIBUTE( "boundingSphereCenter", m_boundingSphereCenter, "The center of the minimum bounding sphere of the model in local coordinates", Be::READWRITE | Be::PERSIST )
		
		MAP_ATTRIBUTE( "boundingSphereRadius", m_boundingSphereRadius, "The radius of the minimum bounding sphere of the model", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "lodLevel", m_lodLevel, "Current lod-level of this spaceobject\n", Be::READ )

		MAP_ATTRIBUTE( "customMask", m_customMask, "\n", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE
		(
			"children",
			m_children,
			"",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		( 
			"curveSets", 
			m_curveSets, 
			"Curvesets for animating things", 
			Be::READWRITE | Be::PERSIST
		)
		
		MAP_ATTRIBUTE( "isAnimated", m_isAnimated, "Indicate if object is animated", Be::READ )

		MAP_METHOD_AND_WRAP( "GetBoneCount", GetBoneCount, "Returns the number of bones in the granny." )
		MAP_METHOD_AND_WRAP
		( 
			"GetBoundingSphereRadius", 
			GetBoundingSphereRadius, 
			"Returns the bounding sphere radius."
		)	
		MAP_METHOD_AND_WRAP
		(
			"GetBoundingSphereCenter",
			GetBoundingSphereCenter,
			"Returns the bounding sphere center."
		)
		
		MAP_METHOD_AND_WRAP
		(
			"CalculateSkinnedBoundingSphere", 
			CalculateSkinnedBoundingSphere, 
			"Calculate and return skinned bounding sphere."
		)
		MAP_METHOD_AND_WRAP
		(
			"CalculateSkinnedBoundingBoxFromTransform", 
			CalculateSkinnedBoundingBoxFromTransform, 
			"Calculate and return bounding spehre of the object projected by the transform provided."
		)

		MAP_METHOD_AND_WRAP
		( 
			"RebuildBoundingSphereInformation", 
			RebuildBoundingSphereInformation, 
			"Rebuilds the bounding data."
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetLocalBoundingBox", 
			GetLocalBoundingBoxFromScript, 
			"Returns the bounding box of the object in local coordinates."
		)
		MAP_METHOD_AND_WRAP
		(
			"PlayAnimation",
			PlayAnimationOnce,
			"PlayAnimation( animName )\n\nPlays the given animation, replacing whatever animation was playing before."
		)
		MAP_METHOD_AND_WRAP
		(
			"PlayAnimationEx",
			PlayAnimationEx,
			"PlayAnimation( animName, loopCount, delay, speed )\n\n"
			"Plays the given animation, replacing whatever animation was playing before.\n"
			"loopCount can be 0 to loop forever.\n"
			"delay is time (in seconds) from now before animation should start playing.\n"
			"speed can be used speed up or slow down playback - use negative values to play backwards.\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"ChainAnimation",
			ChainAnimation,
			"ChainAnimation( animName )\n\nPlays the given animation, starting when currently playing animation finishes."
			"If it is looping then it is replaced at the end of the current loop."
		)
		MAP_METHOD_AND_WRAP
		(
			"ChainAnimationEx",
			ChainAnimationEx,
			"ChainAnimation( animName, loopCount, delay, speed )\n\n"
			"Plays the given animation, starting when currently playing animation finishes."
			"If it is looping then it is replaced at the end of the current loop."
			"loopCount can be 0 to loop forever.\n"
			"delay is time (in seconds) from now before animation should start playing.\n"
			"speed can be used speed up or slow down playback - use negative values to play backwards.\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"EndAnimation",
			EndAnimation,
			"EndAnimation()\n\n"
			"Stops currently playing animation at the end of the current loop iteration."
		)
		MAP_METHOD_AND_WRAP
		(
			"ClearAnimations",
			ClearAnimations,
			"ClearAnimations()\n\n"
			"Abruptly ends all animations."
		)
		MAP_METHOD_AND_WRAP
		(
			"FreezeHighDetailMesh",
			FreezeHighDetailMesh,
			"Freezes the high detail mesh and prevents LOD selection or resource unloading."
		)

		MAP_ATTRIBUTE
		(
			"overlayEffects",
			m_overlayEffects,
			"A list of effects that are added to the current LOD. Rendered additive.",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"animationSequencer",
			m_animationSequencer,
			"",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"animationUpdater",
			m_animationUpdater,
			"Granny animation exposure",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"positionDelta",
			m_positionDelta,
			"Change in global position of the object during the frame",
			Be::READ
		)

		MAP_METHOD_AND_WRAP( "GetDamageLocatorCount", GetDamageLocatorCount, "Get number of damage locators on this ship" )
		MAP_METHOD_AND_WRAP( "GetDamageLocator", GetDamageLocator, "Get the position of indexed damage locator, (0,0,0) is returned for indices out of range." )
		MAP_METHOD_AND_WRAP( "GetTransformedDamageLocator", GetTransformedDamageLocator, "Get the position of indexed damage locator, (0,0,0) is returned for indices out of range." )

		// Temporarily need this to get camera LookAt working. Shouldn't be here for too long <Logi, dec 2012>
		MAP_PROPERTY_READONLY
		(
			"modelWorldPosition",
			GetModelWorldPosition,
			"The spaceobject's model world position"
		)
		
		
    EXPOSURE_END()
}
