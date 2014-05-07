#include "StdAfx.h"

#include "EveTransform.h"

BLUE_DEFINE( EveTransform );
BLUE_DEFINE_INTERFACE( IEveTransform );

const Be::ClassInfo* EveTransform::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveTransform, "" )
        MAP_INTERFACE( EveTransform )
		MAP_INTERFACE( IEveTransform )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( ITr2Pickable )
		MAP_INTERFACE( IWorldPosition )
		MAP_INTERFACE( IUnloadable )

		MAP_ATTRIBUTE
		( 
			"debugShowBoundingBox", 
			m_debugShowBoundingBox, 
			"If set, bounding box is shown if scene is showing debug info.", 
			Be::READWRITE | Be::PERSIST
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
			"hideOnLowQuality", 
			m_hideOnLowQuality,
			"Disables this whole transform and all of it's children when low quaility is selected.", 
			Be::READWRITE | Be::PERSIST 
		)

		MAP_ATTRIBUTE
		(
			"visibilityThreshold",
			m_visibilityThreshold,
			"If the transform holds a mesh, it is only rendered if its estimated pixel\n"
			"diameter is above this threshold. Note that rendering of the children is"
			"not affected. Also setting this to -1.0 disables culling.\n",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"children",
			m_children,
			"",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"particleEmitters",
			m_particleEmitters,
			"A list of emitters owned by this transform",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"particleSystems",
			m_particleSystems,
			"A list of particle systems owned by this transform",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		( 
			"observers", 
			m_observers, 
			"Observers for pushing data between modules every frame. Currently used to push locator data out to the audio2 module.",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"particleEmittersGPU",
			m_particleEmittersGPU,
			"A list of GPU particle emitters owned by this transform",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"useLodLevel",
			m_useLodLevel,
			"Use the lodLevel to downscale this object",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"lodLevel",
			m_lodLevel,
			"Current LOD level, 1(high) to 3(low)",
			Be::READ
		)

    EXPOSURE_CHAINTO( Tr2Transform )
}
