// Copyright © 2011 CCP ehf.

#include "StdAfx.h"
#include "Tr2ParticleTurbulenceForce.h"

BLUE_DEFINE( Tr2ParticleTurbulenceForce );

const Be::ClassInfo* Tr2ParticleTurbulenceForce::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ParticleTurbulenceForce, "" )
		MAP_INTERFACE( ITr2ParticleForce )
		MAP_INTERFACE( Tr2ParticleTurbulenceForce )

		MAP_ATTRIBUTE( "amplitude", m_amplitude, "turbulence amplitude", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "frequency", m_frequency, "turbulence frequency (w - time)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseLevel", m_noiseLevel, "turbulence noise level (octave count)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseRatio", m_noiseRatio, "turbulence noise ratio (octave amplitude coefficient)", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
