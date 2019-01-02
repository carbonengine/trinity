#include "StdAfx.h"
#include "EvePlanet.h"
#include "EveTransform.h"

BLUE_DEFINE( EvePlanet );

const Be::ClassInfo* EvePlanet::ExposeToBlue()
{
    EXPOSURE_BEGIN( EvePlanet, "" )
        MAP_INTERFACE( EvePlanet )
		MAP_INTERFACE( IWorldPosition )
		MAP_INTERFACE( ITr2SecondaryLightSource )
		MAP_INTERFACE( ITriTargetable )

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
			"If set, this transform hierarchy is displayed.\n"
			"Note that turning off display does not automatically turn\n"
			"off update.",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"update",
			m_update,
			"If set, this transform hierarchy is updated every frame.",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(  
			"scaling",
			m_scaling,
			"Uniform scaling applied to object when rendered",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(  
			"radius",
			m_radius,
			"The planet's radius",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(  
			"albedoColor",
			m_albedoColor,
			"Planet albedo color, used for secondary lighting",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(  
			"emissiveColor",
			m_emissiveColor,
			"Planet emissive color",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		( 
			"highDetail", 
			m_highDetail, 
			"Object used for rendering planet in high detail", 
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

#if BLUE_WITH_PYTHON
		MAP_ATTRIBUTE
		(
			"resourceCallback", 
			m_pythonResourceCallback, 
			"A callback function that generates any resources that need lazy loading",
			Be::READWRITE
		)
#endif


		MAP_METHOD_AND_WRAP
		(
			"PrepareForWarp",
			PrepareForWarp,
			":param minDist: minimal distance\n"
			":param dest: destination position"
		)

		MAP_METHOD_AND_WRAP
		(
			"WarpStopped",
			WarpStopped,
			""
		)

		MAP_ATTRIBUTE 
		(
			"zOnlyModel",
			m_zOnlyModel,
			"A model that is used to write to the z-buffer only",
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
			"requiredTextureSize",
			m_requiredTextureSize,
			"",
			Be::READ
		)
		MAP_ATTRIBUTE
		(
			"currentTextureSize",
			m_currentTextureSize,
			"",
			Be::READ
		)
		MAP_ATTRIBUTE
		(
			"warping",
			m_warpMode,
			"",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"estimatedPixelDiameter",
			m_estimatedPixelDiameter,
			"",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"estimatedMaxPixelDiameter",
			m_estimatedMaxPixelDiameter,
			"",
			Be::READ
		)
		

    EXPOSURE_END()
}
