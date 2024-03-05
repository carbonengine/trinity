#include "StdAfx.h"
#include "EveEffectRoot2.h"

BLUE_DEFINE( EveEffectRoot2 );

const Be::ClassInfo* EveEffectRoot2::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveEffectRoot2, "" )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( ITr2SecondaryLightSource )
		MAP_INTERFACE( ITriTargetable )
		MAP_INTERFACE( ITr2CurveSetOwner )
		MAP_INTERFACE( IEveEffectChildrenOwner )
		MAP_INTERFACE( ITr2ControllerOwner )
		MAP_INTERFACE ( IShaderConfigurer )
		MAP_INTERFACE( ITr2SoundEmitterOwner )
		MAP_INTERFACE( ITr2LightOwner )
		MAP_INTERFACE( IWorldPosition )
		MAP_INTERFACE( EveEntity )

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
			Be::READWRITE | Be::PERSIST | Be::NOTIFY
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
			"lodLevel",
			m_lodLevel,
			"",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"dynamicLOD",
			m_dynamicLODSelection,
			"",
			Be::READWRITE | Be::PERSIST
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

		MAP_ATTRIBUTE
		(    
			"modelTranslationCurve",
			m_modelTranslation,
			"Used to add animated translations to ships",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(    
			"modelRotationCurve",
			m_modelRotation,
			"Used to add rotations to the basic rotation curve",
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
			"effectChildren",
			m_effectChildren,
			"",
			Be::READ | Be::PERSIST
		)
		
		MAP_ATTRIBUTE
		( 
			"observers", 
			m_observers, 
			"Observers for pushing data between modules every frame. Currently used to push locator data out to the audio2 module.",
			Be::READ | Be::PERSIST
		)

		MAP_METHOD_AND_WRAP( "GetBoundingSphereRadius", GetBoundingSphereRadius, "Returns the bounding sphere radius." )

		MAP_ATTRIBUTE( "lights", m_lights, "List of dynamic lights", Be::READ | Be::PERSIST );

		MAP_ATTRIBUTE( "externalParameters", m_externalParameters, "List of external parameters to bind to object elements", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "curveSets", m_curveSets, "Curvesets for animating things", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "controllers", m_controllers, "List of object controllers", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( 
			"secondaryLightingSphereRadius", 
			m_secondaryLightingSphereRadiusLocal, 
			"Radius of secondary light source", 
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"secondaryLightingEmissiveColor", 
			m_secondaryLightingEmissiveColor, 
			"Color of the secondary light source", 
			Be::READWRITE | Be::PERSIST )
		MAP_METHOD_AND_WRAP( 
			"Start", 
			Start, 
			"Play all top level curveSets\n"
			":jessica-favorite:\n"
			":jessica-icon: timeline/play.png" )
		MAP_METHOD_AND_WRAP( 
			"Stop", 
			Stop, 
			"Stop all top level curveSets.\n" 
			":jessica-favorite:\n"
			":jessica-icon: timeline/stop.png" )
			
		MAP_METHOD_AND_WRAP( "GetBoundingSphereRadius", GetBoundingSphereRadius, "Returns the bounding sphere radius." )	

		MAP_METHOD_AND_WRAP(
			"SetControllerVariable",
			SetControllerVariable,
			"Set variable for all applicable controllers\n"
			":param name: variable name\n"
			":param value: new variable value\n"
		)

		MAP_METHOD_AND_WRAP(
			"HandleControllerEvent",
			HandleControllerEvent,
			"Pass an event to controllers\n"
			":param name: event name"
		)

		MAP_METHOD_AND_WRAP(
			"StartControllers",
			StartControllers,
			"Start all controllers"
		)
		MAP_METHOD_AND_WRAP
		(
			"FreezeHighDetailMesh",
			FreezeHighDetailMesh,
			""
		)

        MAP_METHOD_AND_WRAP(
            "SetProceduralContainerVariable",
            SetProceduralContainerVariable,
            "Set variable for all applicable ProceduralContainer\n"
            ":param name: variable name\n"
            ":param value: new variable value\n"
        )

    EXPOSURE_END();
}
