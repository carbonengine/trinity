// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightAttributeModifierCameraDependency.h"

BLUE_DEFINE( EveSmartLightAttributeModifierCameraDependency );

const Be::ClassInfo* EveSmartLightAttributeModifierCameraDependency::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSmartLightAttributeModifierCameraDependency, ":jessica-icon: video\n:jessica-icon-color: (112, 192, 29)\n" )
		MAP_INTERFACE( EveSmartLightAttributeModifierCameraDependency )
		MAP_INTERFACE( EveSmartLightAttributeModifierBucket )
		MAP_INTERFACE( IEveSmartLightGroupAttributeModifier )

		MAP_ATTRIBUTE( "name", m_name, "organize your buckets", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "attributeModifiers", m_attributeModifiers, "list of attribute modifiers", Be::READ | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "minimumActivation", m_minimumActivation, "when no condition are met, this is the strength", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "maximumActivation", m_maximumActivation, "when all conditions are met, this is the strength ", Be::READWRITE | Be::PERSIST );

		MAP_ATTRIBUTE( "useCameraDistance", m_useCameraDistance, "fade-in/out modifiers based on distance to camera\n:jessica-group: CameraDistance", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "inverseDistanceFormula", m_inverseDistanceFormula, "if you want to increase the strength on moving further away instead\n:jessica-group: CameraDistance", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "minimumDistance", m_minimumDistance, "at what distance to start changing intensity\n:jessica-group: CameraDistance", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "maximumDistance", m_maximumDistance, "at what distance to stop changing intensity\n:jessica-group: CameraDistance", Be::READWRITE | Be::PERSIST );

		MAP_ATTRIBUTE( "useCameraLookAt", m_useCameraLookAt, "if you want to change intencity based on if you are looking at the object\n:jessica-group: CameraLookAt", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "inverselookAtFormula", m_inverselookAtFormula, "if want intencity to be at full strength when NOT looking at your object\n:jessica-group: CameraLookAt", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "lookAtIntensity", m_lookAtIntencity, "to have the falloff be non-linear over range (fast or slow from center)\n:jessica-group: CameraLookAt\n:jessica-numeric-range: (0.1,10.0)", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "lookAtVisionCone", m_lookAtVisionCone, "degree range at which we modify attributeStrength \n:jessica-group: CameraLookAt\n:jessica-numeric-range: (0.1,10.0)", Be::READWRITE | Be::PERSIST );

		MAP_ATTRIBUTE( "useCameraPlacement", m_useCameraPlacement, "increase activation strength when you are in front of the object\n:jessica-group: cameraPlacement", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "inversePlacementFormula", m_inversePlacementFormula, "increase activation strength when you are NOT in front of the object\n:jessica-group: cameraPlacement", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "placementIntensity", m_placementIntencity, "to have the falloff be non-linear\n:jessica-group: cameraPlacement\n:jessica-numeric-range: (0.1,10.0)", Be::READWRITE | Be::PERSIST );

		MAP_ATTRIBUTE( "attributeMultiplier", m_activationStrength, "scale modifier effect\n:jessica-group: AttributeModifierBlending", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "finalAttributeMultiplier", m_finalActivationStrength, "to visualize/debug compound activationStrength\n:jessica-group: AttributeModifierBlending", Be::READ )

		MAP_ATTRIBUTE( "overwritePosition", m_overwritePosition, "want to have the instances react to a static point instead of their own position?\n:jessica-group: overwriteObjectData", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "overwriteObjectDirection", m_overwriteDirection, "want to have the instances react to a static angle instead of their own?\n:jessica-group: overwriteObjectData", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "positionOverwrite", m_positionOverwrite, "set the overwrite value of the point\n:jessica-group: overwriteObjectData", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "angleOverwrite", m_angleOverwrite, "set the overwrite value of the angle\n:jessica-group: overwriteObjectData", Be::READWRITE | Be::PERSIST );

	EXPOSURE_END()
}
