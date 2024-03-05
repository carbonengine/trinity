#include "StdAfx.h"
#include "EveChildEffectPropagator.h"


Be::VarChooser PropagationChooser[] =
{
	{ "localLocators", BeCast( EveChildEffectPropagator::LOCAL_LOCATORS ), "just place your triggers manually" },
	{ "externalLocatorSet", BeCast( EveChildEffectPropagator::LOCATOR_SET_BY_REF ), "use a parent's locatorSet to propagate" },
	{ "randomSpread", BeCast( EveChildEffectPropagator::RANDOM_SPREAD ), "spreads locators randomly" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "EffectPropagationType", EveChildEffectPropagator::PropagationType, PropagationChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

Be::VarChooser PropagationTriggerChooser[] =
{
	{ "triggerSphereCurve", BeCast( EveChildEffectPropagator::TRIGGER_SPHERE_CURVE ), "animate a radius to trigger the effect" },
	{ "intervalTriggers", BeCast( EveChildEffectPropagator::INTERVAL_TRIGGERS ), "continuous effect trigger" },
	{ "instantPermanent", BeCast( EveChildEffectPropagator::INSTANT_PERMANENT ), "propagate instantly over set and no clean-up" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "EffectPropagationTriggerType", EveChildEffectPropagator::TriggerType, PropagationTriggerChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

BLUE_DEFINE( EveChildEffectPropagator );

const Be::ClassInfo* EveChildEffectPropagator::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveChildEffectPropagator, "Specialized explosion propagator object child" )
        MAP_INTERFACE( EveChildEffectPropagator )
        MAP_INTERFACE( EveChildContainer )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( EveEntity ) 

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "effect", m_effect, "childInstanceContainer", Be::PERSISTONLY | Be::NOTIFY )
		MAP_PROPERTY( "effect", GetEffect, SetEffect, "The instance container that manages the instances" )

		MAP_ATTRIBUTE( "triggerSphereRadiusCurve",	m_triggerSphereRadiusCurve, "Manage the the triggering of effects based on a distance from the triggerSphereOffset", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "localLocators", m_localLocators, "locators for a self-contained propagation", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "curveSets", m_curveSets, "", Be::READ | Be::PERSIST )
		
		MAP_ATTRIBUTE_WITH_CHOOSER( "propagationType", m_type, "", Be::READWRITE | Be::PERSIST | Be::ENUM | Be::NOTIFY, PropagationChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "triggerMethood", m_triggerMethod, "", Be::READWRITE | Be::PERSIST | Be::ENUM | Be::NOTIFY, PropagationTriggerChooser )

		MAP_ATTRIBUTE( "trigger", m_trigger, "reset and start", Be::READWRITE )
		MAP_ATTRIBUTE( "isPlaying", m_isPlaying, "Is the effect playing", Be::READ )
		MAP_ATTRIBUTE( "playTime", m_playTime, "Time since the start of playback", Be::READ )
		MAP_ATTRIBUTE( "triggerSphereScalarMulti", m_triggerSphereScalarMulti, "multiplied by the scalar curve", Be::READ )
		
		MAP_ATTRIBUTE( "completeness", m_completeness, "range: [0:1] ~to use if you don't want 100% of a Locset. Doesn't work for randomSpread\n:jessica-group: SpawnSettings", Be::READWRITE | Be::PERSIST |Be::NOTIFY )
		MAP_ATTRIBUTE( "triggerSphereOffset", m_triggerSphereOffset, "Centerpoint of the trigger sphere\n:jessica-group: SpawnSettings", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "effectScaling", m_effectScaling, "general Vec3 to enlarge the effects (set on spawn)\n:jessica-group: SpawnSettings", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "randScaleMin", m_randScaleMin, "additional randomness -> scaling range\n:jessica-group: SpawnSettings", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "randScaleMax", m_randScaleMax, "additional randomness -> scaling range\n:jessica-group: SpawnSettings", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "stopToClearDelay", m_stopToClearDelay, "delay between: curve finished playing -> the end \n:jessica-group: Looping (TriggerSphereCurve)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "skipCleanup", m_skipCleanup, "use to only propagate once and then idle the propagator (mutually exclusive with replayAfterDelay)\n:jessica-group: Looping (TriggerSphereCurve)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "replayAfterDelay", m_replayAfterDelay, "replay after curve finishes playing + the stopToClearDelay (mutually exclusive with skipCleanup)\n:jessica-group: Looping (TriggerSphereCurve)", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "numTriggers", m_numTriggers, "totall number of spawned locators\n:jessica-group: RandomLocators", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "range", m_rndRange, "total range locators can spawn in\n:jessica-group: RandomLocators", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "minRangeThreshold", m_rndMinRangeThreshold, "should they always spawn at least x meters away from center? \n:jessica-group: RandomLocators", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "ClosenessPreference", m_rndClosenessPreference, "distribution point. (closer to 0/1 more similar)\n:jessica-group: RandomLocators", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		
		MAP_ATTRIBUTE( "locatorSetName", m_locatorSetName, "name of the locator set\n:jessica-group: ExternalLocatorSet", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		
		MAP_ATTRIBUTE( "frequency", m_frequency, "triggers per sec\n:jessica-group: intervalTriggers", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::NOTIFY )
		MAP_ATTRIBUTE( "durationPerEffect", m_effectDuration, "how long until per instance cleanup\n:jessica-group: intervalTriggers", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "stopAfterNumTriggers", m_stopAfterNumTriggers, "-1 to never stop else it stops after [num] creations \n:jessica-group: intervalTriggers", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_METHOD_AND_WRAP( "RebuildLocalTransform", RebuildLocalTransform, "Rebuilds local transform." )
		MAP_METHOD_AND_WRAP( "Stop", Stop, "Stops effect playback.\n:jessica-favorite:\n:jessica-icon: timeline/stop.png" )
	
    EXPOSURE_END()
}

