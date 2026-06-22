// Copyright © 2012 CCP ehf.

#include "StdAfx.h"
#include "Tr2ForceSphereVolume.h"

BLUE_DEFINE( Tr2ForceSphereVolume );

const Be::ClassInfo* Tr2ForceSphereVolume::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ForceSphereVolume, "" )
		MAP_INTERFACE( ITr2ParticleForce )
		MAP_INTERFACE( Tr2ForceSphereVolume )

		MAP_ATTRIBUTE( "forces", m_forces, "List of forces subject to volume", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "position", m_position, "Sphere position", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "radius", m_radius, "Sphere radius", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "exponent", m_exponent, "Falloff function exponent", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
