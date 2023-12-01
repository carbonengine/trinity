#include "StdAfx.h"
#include "EveRootTransform.h"

BLUE_DEFINE( EveRootTransform );

const Be::ClassInfo* EveRootTransform::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveRootTransform, "" )
        MAP_INTERFACE( EveRootTransform )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( ITriTargetable )
		MAP_INTERFACE( ITr2Pickable )
		MAP_INTERFACE( IWorldPosition )

		MAP_ATTRIBUTE
		(    
			"translationCurve",
			m_ballPosition,
			"Vector function slot for attaching a destiny ball to set the position of an object",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(    
			"rotationCurve",
			m_ballRotation,
			"Quaternion function slot for attaching a destiny ball to set the rotation of an object",
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
			"children",
			m_children,
			"",
			Be::READ | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"boundingSphereRadius",
			m_boundingSphereRadius,
			"authorable bounding sphere radius, used by the camera in Eve",
			Be::READWRITE | Be::PERSIST
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
			"lodLevel",
			m_lodLevel,
			"the object's active LOD",
			Be::READ
		)
		MAP_METHOD_AND_WRAP( "GetBoundingSphereRadius", GetBoundingSphereRadius, "Returns the bounding sphere radius." )	

    EXPOSURE_CHAINTO( Tr2Transform )

	// Note that the chaining skips over EveTransform. This is done to convince Jessica
	// to only allow EveTransform objects as children of this, not other EveRootTransform
	// objects.
}
