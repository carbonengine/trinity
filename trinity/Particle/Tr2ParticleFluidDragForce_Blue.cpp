// Copyright © 2010 CCP ehf.

#include "StdAfx.h"
#include "Tr2ParticleFluidDragForce.h"

BLUE_DEFINE( Tr2ParticleFluidDragForce );

const Be::ClassInfo* Tr2ParticleFluidDragForce::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ParticleFluidDragForce, "" )
		MAP_INTERFACE( ITr2ParticleForce )
		MAP_INTERFACE( Tr2ParticleFluidDragForce )

		MAP_ATTRIBUTE( "drag", m_dragConstant, "Applies a drag force: -bV^2 where V is the velocity of a particle", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
