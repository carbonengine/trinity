#include "StdAfx.h"
#include "EveChildLightingOverride.h"


BLUE_DEFINE_INTERFACE( IEveLightingOverride );


BLUE_DEFINE( EveChildLightingOverride );

const Be::ClassInfo* EveChildLightingOverride::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildLightingOverride, "" )
		MAP_INTERFACE( EveEntity )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( IEveLightingOverride )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITr2DebugRenderable )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"priority",
			m_overrides.priority,
			"Priority of this override. Affects blending between different override objects",
			Be::READWRITE | Be::PERSIST | Be::ENUM,
			PostProcessEnums::Tr2PostProcessPriorityChooser )
		MAP_ATTRIBUTE(
			"intensity",
			m_intensity,
			"Manually adjustable intensity for fog settings\n"
			":jessica-numeric-range: (0.0, 1.0)",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"sunColor",
			m_overrides.value.sunColor,
			"Sun color override",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"sunIntensity",
			m_overrides.value.sunIntensity,
			"Sun color intensity: scales the sun color",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"backgroundIntensity",
			m_overrides.value.backgroundIntensity,
			"Background effect intensity override",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"reflectionIntensity",
			m_overrides.value.reflectionIntensity,
			"Overall reflection intensity override",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"volumes",
			m_volumes,
			"The volumes that affect the light overrides",
			Be::READ | Be::PERSIST )
	EXPOSURE_END()
}
