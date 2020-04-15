#include "StdAfx.h"
#include "EveChildEffectPropagator.h"


Be::VarChooser PropagationChooser[] =
{
	{ "localLocators", BeCast( EveChildEffectPropagator::LOCAL_LOCATORS ), "just place your triggers manually" },
	{ "externalLocatorSet", BeCast( EveChildEffectPropagator::LOCATOR_SET_BY_REF ), "point me to an asset and declare it's LocSet name" },
	{ "randomSpread", BeCast( EveChildEffectPropagator::RANDOM_SPREAD ), "spreads locators randomly" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "EffectPropagationType", EveChildEffectPropagator::PropagationType, PropagationChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

BLUE_DEFINE( EveChildEffectPropagator );

const Be::ClassInfo* EveChildEffectPropagator::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveChildEffectPropagator, "Specialized explosion propagator object child" )
        MAP_INTERFACE( EveChildEffectPropagator )
        MAP_INTERFACE( EveChildContainer )
		MAP_INTERFACE( IEveSpaceObjectChild )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( 
			"effect", 
			m_effect,
			"childInstanceContainer", 
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"triggerSphereRadiusCurve",
			m_triggerSphereRadiusCurve,
			"Manage the the triggering of effects based on a distance from the triggerSphereOffset",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"localLocators",
			m_localLocators,
			"locators for a self-contained propagation",
			Be::READWRITE | Be::PERSIST )
		

		MAP_ATTRIBUTE_WITH_CHOOSER( "propagationType", m_type, "", Be::READWRITE | Be::PERSIST | Be::ENUM | Be::NOTIFY, PropagationChooser )
	
		MAP_ATTRIBUTE( 
			"isPlaying", 
			m_isPlaying, 
			"Is the effect playing", 
			Be::READ )
	
		MAP_ATTRIBUTE( 
			"playTime", 
			m_playTime, 
			"Time since the start of playback", 
			Be::READ )
	
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
	
		MAP_ATTRIBUTE( "trigger", m_trigger, "reset and start", Be::READWRITE )
		MAP_ATTRIBUTE( "triggerSphereScalarMulti", m_triggerSphereScalarMulti, "multiplied by the scalar curve", Be::READ )
		
		MAP_ATTRIBUTE( "completeness", m_completeness, "range: [0:1] ~to use if you don't want 100% of a Locset. Doesn't work for randomSpread\n:jessica-group: SpawnSettings", Be::READWRITE | Be::PERSIST |Be::NOTIFY )
		MAP_ATTRIBUTE( "triggerSphereOffset", m_triggerSphereOffset, "Centerpoint of the trigger sphere\n:jessica-group: SpawnSettings", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "effectScaling", m_effectScaling, "general Vec3 to enlarge the effects (set on spawn)\n:jessica-group: SpawnSettings", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "randScaleMin", m_randScaleMin, "additional randomness -> scaling range\n:jessica-group: SpawnSettings", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "randScaleMax", m_randScaleMax, "additional randomness -> scaling range\n:jessica-group: SpawnSettings", Be::READWRITE | Be::PERSIST | Be::NOTIFY )


		MAP_ATTRIBUTE( "stopToClearDelay", m_stopToClearDelay, "delay between: curve finished playing -> the end \n:jessica-group: Looping", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "replayAfterDelay", m_replayAfterDelay, "should this effect loop \n:jessica-group: Looping", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "numTriggers", m_numTriggers, "increase/decrease the size of the mesh render\n:jessica-group: RandomLocators", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "range", m_rndRange, "should mesh face camera \n:jessica-group: RandomLocators", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minRangeThreshold", m_rndMinRangeThreshold, "should they always spawn at least x meters away from center? \n:jessica-group: RandomLocators", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "ClosenessPreference", m_rndClosenessPreference, "distribution point. (closer to 0/1 more similar)\n:jessica-group: RandomLocators", Be::READWRITE | Be::PERSIST )
		
		MAP_ATTRIBUTE( "refObject", m_refObject, "target obj that has the locator set\n:jessica-group: ExternalLocatorSet", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "locatorSetName", m_locatorSetName, "name of the locator set\n:jessica-group: ExternalLocatorSet", Be::READWRITE | Be::PERSIST )
		
		

		MAP_METHOD_AND_WRAP("RebuildLocalTransform", RebuildLocalTransform, "Rebuilds local transform.")

		MAP_METHOD_AND_WRAP( 
			"Play", 
			Play, 
			"Starts effect playback.\n"
			":jessica-favorite:\n"
			":jessica-icon: timeline/play.png" )
		MAP_METHOD_AND_WRAP( 
			"Stop", 
			Stop, 
			"Stops effect playback.\n"
			":jessica-favorite:\n"
			":jessica-icon: timeline/stop.png" )
	
    EXPOSURE_END()
}

