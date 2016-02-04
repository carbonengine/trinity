#include "StdAfx.h"
#include "EveEffectRoot2.h"

BLUE_DEFINE( EveEffectRoot2 );

const Be::ClassInfo* EveEffectRoot2::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveEffectRoot2, "" )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( IInitialize )

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
			"",
			Be::READWRITE | Be::PERSIST
		)

		
		MAP_ATTRIBUTE
		(
			"estimatedSize",
			m_estimatedSize,
			"",
			Be::READ
		)

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

#if BLUE_WITH_PYTHON
		// expose bounding sphere as two variables: center pos and radius
		MAPFLOATARRAYSIZE( "boundingSphereCenter", m_boundingSphere, BlueDefaultIID, "The center of the minimum bounding sphere of the effect in local coordinates", Be::READWRITE | Be::PERSIST, 3 )
#endif

		MAP_ATTRIBUTE( "boundingSphereRadius", m_boundingSphere.w, "The radius of the minimum bounding sphere of the effect", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "duration", m_effectDuration, "", Be::READWRITE | Be::PERSIST )
		
		MAP_ATTRIBUTE
		( 
			"observers", 
			m_observers, 
			"Observers for pushing data between modules every frame. Currently used to push locator data out to the audio2 module.",
			Be::READWRITE | Be::PERSIST
		)

		MAP_METHOD_AND_WRAP( "GetBoundingSphereRadius", GetBoundingSphereRadius, "Returns the bounding sphere radius." )

		MAP_ATTRIBUTE( "lights", m_lights, "List of dynamic lights", Be::READ | Be::PERSIST );

	
		MAP_ATTRIBUTE
		(
			"effectChildren",
			m_effectChildren,
			"",
			Be::READWRITE | Be::PERSIST
		)

    EXPOSURE_END();
}
