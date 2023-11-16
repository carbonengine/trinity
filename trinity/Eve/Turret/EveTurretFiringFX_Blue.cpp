////////////////////////////////////////////////////////////
//
//    Created:   July 2011
//    Copyright: CCP 2011
//
#include "StdAfx.h"
#include "EveTurretFiringFX.h"

BLUE_DEFINE_INTERFACE( IEveFiringEffectElement );

BLUE_DEFINE( EveTurretFiringFX );

const Be::ClassInfo* EveTurretFiringFX::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveTurretFiringFX, "" )
        MAP_INTERFACE( EveTurretFiringFX )
        MAP_INTERFACE( IInitialize )
        MAP_INTERFACE( INotify )
        MAP_INTERFACE( ITr2ControllerOwner )
		
		MAP_ATTRIBUTE( "name", m_name, "A name for this firing effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "Toggle rendering", Be::READWRITE )

		MAP_ATTRIBUTE( "useMuzzleTransform", m_useMuzzleTransform, "Set this and the stretch effect aims from data in the muzzle transform.", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "isFiring", m_isFiring, "Is this effect rendering (because it is firing)", Be::READ )
		MAP_ATTRIBUTE( "firingDuration", m_firingDuration, "How long is the firing animation", Be::READ )

		MAP_ATTRIBUTE( "firingDurationOverride", m_firingDurationOverride, "Override default firing duration", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "firingPeakTime", m_firingPeakTime, "After how many seconds is this effect peaking?", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isLoopFiring", m_isLoopFiring, "some turrets (like miners or salvagers) loop the firing effect endlessly", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boneName", m_boneName, "This is the prefix name for the bone this will get attached to", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "firingDelay1", m_perMuzzleData[0].constantDelay, "Delay in seconds for firing effect 1", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "firingDelay2", m_perMuzzleData[1].constantDelay, "Delay in seconds for firing effect 2", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "firingDelay3", m_perMuzzleData[2].constantDelay, "Delay in seconds for firing effect 3", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "firingDelay4", m_perMuzzleData[3].constantDelay, "Delay in seconds for firing effect 4", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "firingDelay5", m_perMuzzleData[4].constantDelay, "Delay in seconds for firing effect 5", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "firingDelay6", m_perMuzzleData[5].constantDelay, "Delay in seconds for firing effect 6", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "firingDelay7", m_perMuzzleData[6].constantDelay, "Delay in seconds for firing effect 7", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "firingDelay8", m_perMuzzleData[7].constantDelay, "Delay in seconds for firing effect 8", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "firingDelay9", m_perMuzzleData[8].constantDelay, "Delay in seconds for firing effect 9", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "firingDelay10", m_perMuzzleData[9].constantDelay, "Delay in seconds for firing effect 10", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "firingDelay11", m_perMuzzleData[10].constantDelay, "Delay in seconds for firing effect 11", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "firingDelay12", m_perMuzzleData[11].constantDelay, "Delay in seconds for firing effect 12", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "stretch", m_stretch, "A list of stretch effects for this firing effect", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "endPosition", m_endPosition, "Destination or end position", Be::READWRITE )
		
		MAP_ATTRIBUTE( "scaleEffectTarget", m_scaleEffectTarget, "Toggle whether the firing effect target object is scaled by the target object", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minRadius", m_minRadius, "Used for scaling the firing effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxRadius", m_maxRadius, "Used for scaling the firing effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minScale", m_minScale, "Used for scaling the firing effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxScale", m_maxScale, "Used for scaling the firing effect", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "startCurveSet", m_startCurveSet, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "stopCurveSet", m_stopCurveSet, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE
		(
			"sourceObserver",
			m_sourceObserver,
			"Observer at the source position",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"destinationObserver",
			m_destinationObserver,
			"Observer at the destination position",
			Be::READWRITE | Be::PERSIST
		)

		MAP_METHOD_AND_WRAP(
			"GetPerMuzzleEffectCount",
			GetPerMuzzleEffectCount,
			"Return the number of muzzles in this firing effect." )

	EXPOSURE_END()
}



