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
		MAP_INTERFACE( IInitialize )

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
			Be::READ | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"particleEmitters",
			m_particleEmitters,
			"A list of emitters owned by this transform",
			Be::READ | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"particleSystems",
			m_particleSystems,
			"A list of particle systems owned by this transform",
			Be::READ | Be::PERSIST
		)
		MAP_ATTRIBUTE
		( 
			"observers", 
			m_observers, 
			"Observers for pushing data between modules every frame. Currently used to push locator data out to the audio2 module.",
			Be::READ | Be::PERSIST
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

		MAP_ATTRIBUTE
		(
			"meshLod",
			m_meshLod,
			":jessica-hidden: True",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"overrideBoundsMin",
			m_overrideBoundsMin,
			"",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"overrideBoundsMax",
			m_overrideBoundsMax,
			"",
			Be::READWRITE | Be::PERSIST
		)

    EXPOSURE_CHAINTO( Tr2Transform )
}
