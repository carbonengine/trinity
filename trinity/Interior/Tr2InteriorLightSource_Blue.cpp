#include "StdAfx.h"

#include "Tr2InteriorLightSource.h"
#include "TriConstants.h"
#include "Resources/TriTextureRes.h"

BLUE_DEFINE( Tr2InteriorLightSource );

const Be::ClassInfo* Tr2InteriorLightSource::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2InteriorLightSource, "" )
        MAP_INTERFACE( Tr2InteriorLightSource )
        MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( ITr2InteriorLight )

		MAP_ATTRIBUTE( "name", m_name, "The name of this interior light source", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "position", m_position, "Position of light emitter", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "radius", m_radius, "Maximum distance from emitter", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "color", m_color, "The light's color", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "kelvinColor", m_kelvinColor, "Kelvin color temperature", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "useKelvinColor", m_useKelvinColor, "Use Kelvin color or RGB?", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "falloff", m_falloff, "Exponential falloff for distance from emitter", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "specularIntensity", m_specularIntensity, "Additional specular light intensity multiplier", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "coneAlphaOuter", m_coneAlphaOuter, "A spotlight's outer cone angle (Everything higher than 90 degrees results in a pointlight!)", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "coneAlphaInner", m_coneAlphaInner, "A spotlight's inner cone angle", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "coneDirection", m_coneDirection, "A spotlight's direction", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "primaryLighting", m_primaryLighting, "Does this lightsource contribute to primary lighting?", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "curveSets", m_curveSets, "Curve sets to animate light attributes", Be::READ | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "IsSpotLight", IsSpotLight, "Returns true if the light is a spot light (cone angle < 90 degrees) ")

	EXPOSURE_END()
}

