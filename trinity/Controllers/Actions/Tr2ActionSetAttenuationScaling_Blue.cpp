////////////////////////////////////////////////////////////
//
//    Created:   May 2020
//    Copyright: CCP 2020
//

#include "StdAfx.h"
#include "Tr2ActionSetAttenuationScaling.h"


BLUE_DEFINE( Tr2ActionSetAttenuationScaling );

const Be::ClassInfo* Tr2ActionSetAttenuationScaling::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ActionSetAttenuationScaling, "" )
		MAP_INTERFACE( Tr2ActionSetAttenuationScaling )
		MAP_INTERFACE( ITr2ControllerAction )

		MAP_ATTRIBUTE( "emitter",
                        m_emitterName,
                        "Emitter name",
                        Be::READWRITE | Be::PERSIST
                     )
		MAP_ATTRIBUTE( "controllerVariable", 
					   m_controllerVariableName, 
					   "Optionally multiply the value of a controller variable by the scaling factor.",
					   Be::READWRITE | Be::PERSIST
				     )
		MAP_ATTRIBUTE( "scalingFactor",
                       m_scalingFactor,
                       "The amount you want to scale attenuation by. Applies to all sounds on an emitter.\n"
                       "Example values:\n"
                       "0.5 = halfing the attenuation range\n"
                       "1.0 = keeping the attenuation range as defined in Wwise\n"
                       "2.0 = Doubling attenuation range \n"
					   "Debug notes: \n"
			           "* You cannot see this visually represented without stopping the sound and starting it again.\n"
					   "* All emitters that are scaled by this will change their color to red",
                       Be::READWRITE | Be::PERSIST 
                     )
		// MAP_PROPERTY_READONLY is called regularly by Jessica so this will be updated at all times in Jessica.
		MAP_PROPERTY_READONLY("finalScalingFactor",
                              GetScalingFactor,
                              "The final scaling factor that will be sent to Wwise."
							  )
		MAP_METHOD_AND_WRAP(
			"StartWithController",
			StartWithController,
			"Starts the action manually without changing state machine state.\n"
			":type controller: Tr2Controller\n"
			":param controller: The controller sent to the start method of the action.\n" )
	EXPOSURE_END()
}
